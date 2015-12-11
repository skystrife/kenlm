#include "util/probing_hash_table.hh"

#include "util/murmur_hash.hh"
#include "util/scoped.hh"

#define BOOST_TEST_MODULE ProbingHashTableTest
#include <boost/test/unit_test.hpp>
#include <boost/scoped_array.hpp>
#include <boost/functional/hash.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

namespace util {
namespace {

struct Entry {
  unsigned char key;
  typedef unsigned char Key;

  unsigned char GetKey() const {
    return key;
  }

  void SetKey(unsigned char to) {
    key = to;
  }

  uint64_t GetValue() const {
    return value;
  }

  uint64_t value;
};

typedef ProbingHashTable<Entry, boost::hash<unsigned char> > Table;

BOOST_AUTO_TEST_CASE(simple) {
  size_t size = Table::Size(10, 1.2);
  boost::scoped_array<char> mem(new char[size]);
  memset(mem.get(), 0, size);

  Table table(mem.get(), size);
  Table::ConstIterator i;
  BOOST_CHECK(!table.Find(2, i));
  Entry to_ins;
  to_ins.key = 3;
  to_ins.value = 328920;
  table.Insert(to_ins);
  BOOST_REQUIRE(table.Find(3, i));
  BOOST_CHECK_EQUAL(3, i->GetKey());
  BOOST_CHECK_EQUAL(static_cast<uint64_t>(328920), i->GetValue());
  BOOST_CHECK(!table.Find(2, i));
}

struct Entry64 {
  uint64_t key;
  typedef uint64_t Key;

  Entry64() {}

  explicit Entry64(uint64_t key_in) {
    key = key_in;
  }

  Key GetKey() const { return key; }
  void SetKey(uint64_t to) { key = to; }
};

struct MurmurHashEntry64 {
  std::size_t operator()(uint64_t value) const {
    return util::MurmurHash64A(&value, 8);
  }
};

typedef ProbingHashTable<Entry64, MurmurHashEntry64> Table64;

BOOST_AUTO_TEST_CASE(Double) {
  for (std::size_t initial = 19; initial < 30; ++initial) {
    size_t size = Table64::Size(initial, 1.2);
    scoped_malloc mem(MallocOrThrow(size));
    Table64 table(mem.get(), size, std::numeric_limits<uint64_t>::max());
    table.Clear();
    for (uint64_t i = 0; i < 19; ++i) {
      table.Insert(Entry64(i));
    }
    table.CheckConsistency();
    mem.call_realloc(table.DoubleTo());
    table.Double(mem.get());
    table.CheckConsistency();
    for (uint64_t i = 20; i < 40 ; ++i) {
      table.Insert(Entry64(i));
    }
    mem.call_realloc(table.DoubleTo());
    table.Double(mem.get());
    table.CheckConsistency();
  }
}

typedef AutoProbing<Entry64, MurmurHashEntry64> AutoTable64;

BOOST_AUTO_TEST_CASE(AutoProbeRandom) {
  AutoTable64 table(5, std::numeric_limits<uint64_t>::max());

  std::vector<uint64_t> values;
  values.reserve(500000);

  boost::random::mt19937_64 rng;
  boost::random::uniform_int_distribution<uint64_t> dist(0, 500000);
  for (int i = 0; i < 500000; ++i)
    values.push_back(dist(rng));

  std::vector<uint64_t> nonvalues;
  nonvalues.reserve(500000);
  boost::random::uniform_int_distribution<uint64_t> dist2(500001, 1000000);
  for (int i = 0; i < 500000; ++i)
    nonvalues.push_back(dist2(rng));

  for (std::size_t i = 0; i < values.size(); ++i)
    table.Insert(Entry64(values[i]));

  for (std::size_t i = 0; i < values.size(); ++i) {
    AutoTable64::ConstIterator it;
    BOOST_CHECK(table.Find(values[i], it));
  }

  for (std::size_t i = 0; i < nonvalues.size(); ++i) {
    AutoTable64::ConstIterator it;
    BOOST_CHECK(!table.Find(nonvalues[i], it));
  }
}

} // namespace
} // namespace util
