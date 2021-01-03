

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"
#include "subscribePipe.hh"

using namespace MediaNet;

SubscribePipe::SubscribePipe(PipeInterface *t) : PipeInterface(t) {}
