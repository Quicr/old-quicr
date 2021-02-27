#include "include/fib.hh"
#include "include/multimap_fib.hh"
#include "include/relay.hh"
#include <iostream>

int main() {
  auto relay = Relay{5004};
  while (1) {
    relay.process();
  }
}