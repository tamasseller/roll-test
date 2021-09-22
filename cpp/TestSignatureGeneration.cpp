#include "RpcSerdes.h"
#include "RpcStlMap.h"
#include "RpcStlSet.h"
#include "RpcStlList.h"
#include "RpcStlTuple.h"
#include "RpcStlArray.h"
#include "RpcCTStr.h"

#include "1test/Test.h"

#include <sstream>

TEST_GROUP(SignatureGenerator) 
{
    template<class... T>
    std::string sgn() 
    {
        static constexpr auto ret = rpc::writeSignature<T...>(""_ctstr);

        auto alt = rpc::writeSignature<T...>("alt"_ctstr);
        CHECK((const char*)ret != (const char*)alt);
        CHECK(std::string((const char*)alt) == std::string("alt") + std::string((const char*)ret));

        std::stringstream ss;
        rpc::writeSignature<T...>(ss);
        CHECK(ss.str() == std::string((const char*)ret));

        return (const char*)ret;
    }
};

TEST(SignatureGenerator, VoidOfNothing) {
    CHECK(sgn<>() == "()");
}

TEST(SignatureGenerator, VoidOfInt) {
    CHECK(sgn<int>() == "(i4)");
}

TEST(SignatureGenerator, VoidOfUint) {
    CHECK(sgn<unsigned int>() == "(u4)");
}

TEST(SignatureGenerator, VoidOfShort) {
    CHECK(sgn<short>() == "(i2)");
}

TEST(SignatureGenerator, VoidOfUshort) {
    CHECK(sgn<unsigned short>() == "(u2)");
}

TEST(SignatureGenerator, VoidOfChar) {
    CHECK(sgn<char>() == "(i1)");
}

TEST(SignatureGenerator, VoidOfUchar) {
    CHECK(sgn<unsigned char>() == "(u1)");
}

TEST(SignatureGenerator, VoidOfLong) {
    CHECK(sgn<long>() == "(i8)");
}

TEST(SignatureGenerator, VoidOfUlong) {
    CHECK(sgn<unsigned long long>() == "(u8)");
}

TEST(SignatureGenerator, VoidOfBool) {
    CHECK(sgn<bool>() == "(b)");
}

TEST(SignatureGenerator, VoidOfTwoInts) {
    CHECK(sgn<int, int>() == "(i4,i4)");
}

TEST(SignatureGenerator, VoidOfTwoFourLongs) {
    CHECK(sgn<long, unsigned long, long long, unsigned long long>() == "(i8,u8,i8,u8)");
}

TEST(SignatureGenerator, VoidOfAllPrimitives) {
    CHECK(sgn<bool, char, unsigned char, short, unsigned short, int, unsigned int, long, unsigned long>() 
        == "(b,i1,u1,i2,u2,i4,u4,i8,u8)");
}

TEST(SignatureGenerator, VoidOfBitVector) {
    CHECK(sgn<std::vector<bool>>() == "([b])");
}

TEST(SignatureGenerator, VoidOfIntList) {
    CHECK(sgn<std::list<int>>() == "([i4])");
}

TEST(SignatureGenerator, VoidOfIntVectorAndUlongList) {
    CHECK(sgn<std::vector<int>, std::list<unsigned long>>() == "([i4],[u8])");
}

TEST(SignatureGenerator, IntOfNothing) {
    CHECK(sgn<rpc::Call<int>>() == "((i4))");
}

TEST(SignatureGenerator, FwdList) {
    CHECK(sgn<std::forward_list<int>>() == "([i4])");
}

TEST(SignatureGenerator, Deque) {
    CHECK(sgn<std::deque<char>>() == "([i1])");
}

TEST(SignatureGenerator, Set) {
    CHECK(sgn<std::set<short>>() == "([i2])");
}

TEST(SignatureGenerator, Multiset) {
    CHECK(sgn<std::multiset<unsigned short>>() == "([u2])");
}

TEST(SignatureGenerator, UnorderedSet) {
    CHECK(sgn<std::unordered_set<long>>() == "([i8])");
}

TEST(SignatureGenerator, UnorderedMultiset) {
    CHECK(sgn<std::unordered_multiset<unsigned long>>() == "([u8])");
}

TEST(SignatureGenerator, Map) {
    CHECK(sgn<std::map<unsigned char, short>>() == "([{u1,i2}])");
}

TEST(SignatureGenerator, MultiMap) {
    CHECK(sgn<std::map<char, unsigned short>>() == "([{i1,u2}])");
}

TEST(SignatureGenerator, UnorderedMap) {
    CHECK(sgn<std::unordered_map<long, int>>() == "([{i8,i4}])");
}

TEST(SignatureGenerator, UnorderedMultiMap) {
    CHECK(sgn<std::unordered_multimap<unsigned long, unsigned int>>() == "([{u8,u4}])");
}

TEST(SignatureGenerator, IntListOfByteVector) {
    CHECK(sgn<std::vector<unsigned char>, rpc::Call<std::list<int>>>() == "([u1],([i4]))");
}

TEST(SignatureGenerator, VoidOfIntCharPair) {
    CHECK(sgn<std::pair<int, char>>() == "({i4,i1})");
}

TEST(SignatureGenerator, VoidOfUshort3tuple) {
    CHECK(sgn<std::tuple<unsigned short, unsigned short, unsigned short>>() == "({u2,u2,u2})");
}

TEST(SignatureGenerator, VoidOfByteVectorStringPairList) {
    CHECK(sgn<std::list<std::pair<std::vector<unsigned char>, std::string>>>() == "([{[u1],[i1]}])");
}

TEST(SignatureGenerator, StringToIntMultimapOfIntToStringMap) {
    CHECK(sgn<std::map<int, std::string>, rpc::Call<std::multimap<std::string, int>>>() == "([{i4,[i1]}],([{[i1],i4}]))");
}

TEST(SignatureGenerator, CharToIntMapOfCharSet) {
    CHECK(sgn<std::set<char>, rpc::Call<std::map<char,int>>>() == "([i1],([{i1,i4}]))");
}

TEST(SignatureGenerator, StringListPlaceholder) {
    CHECK(sgn<rpc::CollectionPlaceholder<rpc::CollectionPlaceholder<char>>>() == "([[i1]])");
}

