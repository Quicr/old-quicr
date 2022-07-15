#include <algorithm>
#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fragmentPipe.hh"
#include "quicr/packet.hh"

using namespace MediaNet;

FragmentPipe::FragmentPipe(PipeInterface* t)
				: PipeInterface(t)
				, mtu(1200)
{}

void
FragmentPipe::updateStat(PipeInterface::StatName stat, uint64_t value)
{
	if (stat == PipeInterface::StatName::mtu) {
		mtu = (int)value;
	}

	PipeInterface::updateStat(stat, value);
}

bool
FragmentPipe::send(std::unique_ptr<Packet> packet)
{
	assert(nextPipe);
	// std::clog << "Frag::Send packet " << *packet << std::endl;

	// TODO  break packets larger than mtu bytes into equal size fragments less

	const int extraHeaderSizeBytes = 25; // TODO tune and move
	const int minPacketPayload = 56;     // TODO move

	if (packet->fullSize() + extraHeaderSizeBytes <= mtu) {
		std::clog << "\t" << packet->shortName()
							<< "[Send: not fragmented,  as size=" << packet->fullSize()
							<< " mtu=" << mtu << "]" << std::endl;
		return nextPipe->send(move(packet));
	}

	ClientData clientData;
	NamedDataChunk namedDataChunk;
	DataBlock datablock;
	EncryptedDataBlock encryptedDataBlock;

	bool ok = true;
	assert(nextTag(packet) == PacketTag::clientData);
	ok &= packet >> clientData;
	ok &= packet >> namedDataChunk;
	assert(ok);
	bool encrypt = true;
	if (nextTag(packet) == PacketTag::encDataBlock) {
		ok &= packet >> encryptedDataBlock;
		assert(ok);
		assert(fromVarInt(encryptedDataBlock.metaDataLen) == 0); // TODO
	} else if (nextTag(packet) == PacketTag::dataBlock) {
		ok &= packet >> datablock;
		assert(ok);
		assert(fromVarInt(datablock.metaDataLen) == 0); // TODO
		encrypt = false;
	} else {
		assert("incorrect next tag");
	}

	assert(namedDataChunk.shortName.fragmentID == 0);

	uint16_t dataSize = mtu - extraHeaderSizeBytes;
	if (dataSize < minPacketPayload) {
		dataSize = minPacketPayload;
	}
	assert(dataSize > 1);

	size_t numDone = 0;
	size_t numLeft = packet->size();
	uint8_t frag = 1;

	while (numLeft > 0) {
		size_t numUse = std::min(size_t(dataSize), numLeft);
		std::unique_ptr<Packet> fragPacket = packet->clone();

		fragPacket->resize(numUse);
		std::copy(&(packet->data()) + numDone,
							&(packet->data()) + numDone + numUse,
							&(fragPacket->data()));
		numDone += numUse;
		numLeft -= numUse;

		fragPacket->setFragID(frag, (numLeft == 0));
		namedDataChunk.shortName = fragPacket->shortName();
		if (encrypt) {
			encryptedDataBlock.cipherDataLen = toVarInt(numUse);
			fragPacket << encryptedDataBlock;
		} else {
			datablock.dataLen = toVarInt(numUse);
			fragPacket << datablock;
		}

		fragPacket << namedDataChunk;
		fragPacket << clientData;

		// std::clog << "\t Frag Packet Name: "<< fragPacket->name
		//					<< ", Size" << fragPacket->size() <<
		//std::endl;

		ok &= nextPipe->send(move(fragPacket));

		frag++;

		assert(frag < 64);
	}

	return ok;
}

std::unique_ptr<Packet>
FragmentPipe::recv()
{
	// TODO - cache fragments and reassemble then pass on the when full packet is
	// received
	assert(nextPipe);

	while (true) {

		auto packet = nextPipe->recv();
		if (!packet) {
			return packet;
		}

		return processRxPacket(std::move(packet));
	}
}

std::unique_ptr<Packet>
FragmentPipe::processRxPacket(std::unique_ptr<Packet> packet)
{
	// TODO - this is broken now, need to looka at shortname to decide if this
	// is a fragment or not

	if (nextTag(packet) != PacketTag::shortName) {
		return packet;
	}

	auto next_tag = nextTag(packet);
	while (next_tag == PacketTag::shortName) {
		NamedDataChunk namedDataChunk;
		EncryptedDataBlock encryptedDataBlock;
		DataBlock datablock;

		bool ok = true;
		bool encrypted = true;
		ok &= packet >> namedDataChunk;
		if (nextTag(packet) == PacketTag::encDataBlock) {
			ok &= packet >> encryptedDataBlock;
		} else if (nextTag(packet) == PacketTag::dataBlock) {
			ok &= packet >> datablock;
			encrypted = false;
		}

		if (!ok) {
			//  TODO log bad packet
			return std::unique_ptr<Packet>(nullptr);
		}

		// put the content back
		if (encrypted) {
			packet << encryptedDataBlock;
		} else {
			packet << datablock;
		}

		if (namedDataChunk.shortName.fragmentID == 0) {
			// packet wasn't fragmented
			// TODO (1): add explicit marking instead of checking fragmentID?
			// TODO (2): hide the pop and push back tag semantics behind an api
			// std::clog << "frag recv: unfragmented:" << namedDataChunk.shortName
			//						<< std::endl;
			// set the contents back
			packet << namedDataChunk;

			return packet;
		}

		uint16_t payloadSize = (encrypted)
													 ? fromVarInt(encryptedDataBlock.cipherDataLen)
													 : fromVarInt(datablock.dataLen);
		assert(packet->size() >= payloadSize); // TODO - change log error

		auto packetCopy = packet->clone();
		packetCopy->name = namedDataChunk.shortName;

		// TODO - set lifetime in packetCopy

		// std::clog << "\t [Frag Recv:" << namedDataChunk.shortName << " size=" <<
		//					packet->size() << std::endl;

		{
			std::lock_guard<std::mutex> lock(fragListMutex);
			// std::clog << "\t [fragList: added:" << namedDataChunk.shortName <<
			// std::endl;
			fragList.emplace(namedDataChunk.shortName, move(packetCopy));
		}

		// TODO - clear out old fragments

		// check if we have all the fragments
		bool haveAll = true;
		int frag = 0;
		int numFrag = 0;
		while (haveAll) {
			frag++;
			// std::cerr << "\t [recv:processing frag: " << frag << std::endl;
			ShortName fragName = namedDataChunk.shortName;
			fragName.fragmentID = frag * 2;
			if (fragList.find(fragName) != fragList.end()) {
				// exists but is not the last fragment
				// std::cerr << "\t [recv: not last frag: " << frag << std::endl;
				continue;
			}
			fragName.fragmentID = frag * 2 + 1;
			if (fragList.find(fragName) != fragList.end()) {
				// exists and is the last element
				numFrag = frag;
				// std::cerr << "\t [recv: last frag: numFrags:" << numFrag <<
				// std::endl;
				break;
			}
			haveAll = false;
		}

		if (haveAll) {
			// form the new packet from all fragments and return
			// std::cerr << "\t [HAVE ALL for: " << namedDataChunk.shortName <<
			//					std::endl;
			ShortName fragName = namedDataChunk.shortName;

			auto result = std::unique_ptr<Packet>(nullptr);

			for (int i = 1; i <= numFrag; i++) {
				fragName.fragmentID = i * 2 + ((i == numFrag) ? 1 : 0);
				auto fragPair = fragList.find(fragName);
				assert(fragPair != fragList.end());
				std::unique_ptr<Packet> fragPacket = move(fragPair->second);
				assert(fragPacket);
				fragList.erase(fragPair);

				EncryptedDataBlock encryptedDataBlock;
				DataBlock datablock;
				if (encrypted) {
					fragPacket >> encryptedDataBlock;
				} else {
					fragPacket >> datablock;
				}

				uint32_t dataSize = (encrypted)
														? fromVarInt(encryptedDataBlock.cipherDataLen)
														: fromVarInt(datablock.dataLen);
				assert(fragPacket->size() > 0);
				assert(dataSize > 0);
				assert(dataSize <= fragPacket->size());

				if (i == 1) {
					// use the first fragment as the result packet
					result = move(fragPacket);
					result->setFragID(0, true);
					assert(dataSize <= result->fullSize());
					result->headerSize = (int)(result->fullSize()) - dataSize;
					continue;
				}

				assert(result);
				assert(fragPacket);
				result->resize((int)(result->size()) + dataSize);

				uint8_t* src = &(fragPacket->data()) + fragPacket->size() - dataSize;
				uint8_t* end = &(fragPacket->data()) + fragPacket->size();
				uint8_t* dst = &(result->data()) + result->size() - dataSize;

				std::copy(src, end, dst);
			}

			assert(result);
			namedDataChunk.shortName.fragmentID = 0;

			if (encrypted) {
				encryptedDataBlock.cipherDataLen = toVarInt(result->size());
				result << encryptedDataBlock;
			} else {
				datablock.dataLen = toVarInt(result->size());
				result << datablock;
			}

			result << namedDataChunk;

			result->name = namedDataChunk.shortName;
			result->setFragID(0, true);
			return result;
		}
		next_tag = nextTag(packet);
	}
	return nullptr;
}

void
FragmentPipe::updateMTU(uint16_t val, uint32_t pps)
{
	mtu = val;

	PipeInterface::updateMTU(val, pps);
}
