#include "RpcSerdes.h"
#include "RpcStlMap.h"
#include "RpcStlSet.h"
#include "RpcStlList.h"
#include "RpcStlTuple.h"
#include "RpcStlArray.h"
#include "RpcStreamReader.h"
#include "RpcArrayWriter.h"
#include "RpcCollectionGenerator.h"
#include "RpcCTStr.h"
#include "RpcStruct.h"

#include "MockStream.h"

#include "1test/Test.h"
#include "TestCallIdAccessor.h"

#include <sstream>

TEST_GROUP(SerDes), rpc::CallIdTestAccessor
{
    using a = std::initializer_list<int>;

    template<class... C>
    auto write(C&&... c)
    {
        MockStream stream(rpc::determineSize(std::forward<C>(c)...));
        auto a = stream.access();
        CHECK(rpc::serialize(a, std::forward<C>(c)...));
        CHECK(!a.write('\0'));
        return std::move(stream);
    }

    template<class... C>
    bool read(MockStream &stream, C&&... c)
    {
        bool done = false;
        auto b = stream.access();
        bool deserOk = nullptr == rpc::deserialize<C...>(b, [&done, exp{std::make_tuple<C...>(std::forward<C>(c)...)}](auto&&... args){
            CHECK(std::make_tuple(args...) == exp);
            done = true;
        });

        return deserOk && done;
    }

    template<class... C>
    void test(C&&... c)
    {
        auto data = write<C...>(std::forward<C>(c)...);
        CHECK(read<C...>(data, std::forward<C>(c)...));
    }
};

TEST(SerDes, Void) { test(); }
TEST(SerDes, Int) { test(int(1)); }
TEST(SerDes, Ints) { test(int(1), int(2)); }
TEST(SerDes, Mixed) { test(int(2147483647), short(4), (unsigned char)5); }
TEST(SerDes, Pair) { test(std::make_pair(char(6), long(1234))); }
TEST(SerDes, Tuple) { test(std::make_tuple(int(8), short(9), char(10))); }
TEST(SerDes, IntList) { test(std::list<int>{11, 12, 13}); }
TEST(SerDes, ULongDeque) { test(std::deque<unsigned long>{123456ul, 234567ul, 3456789ul}); }
TEST(SerDes, UshortForwardList) { test(std::forward_list<unsigned short>{123, 231, 312}); }
TEST(SerDes, ShortVector) { test(std::vector<short>{14, 15}); }
TEST(SerDes, EmptyLongVector) { test(std::vector<long>{}); }
TEST(SerDes, String) { test(std::string("Hi!")); }
TEST(SerDes, CharSet) { test(std::set<char>{'a', 'b', 'c'}); }
TEST(SerDes, CharMultiSet) { test(std::set<char>{'x', 'x', 'x'}); }
TEST(SerDes, IntToCharMap) { test(std::map<int, char>{{1, 'a'}, {2, 'b'}, {3, 'c'}, {4, 'a'}}); }
TEST(SerDes, CharToIntMultimap) { test(std::multimap<char, int>{{'a', 1}, {'b', 2}, {'c', 3}, {'a', 4}}); }
TEST(SerDes, CharUnorderedSet) { test(std::unordered_set<char>{'a', 'b', 'c'}); }
TEST(SerDes, CharUnorderedMultiSet) { test(std::unordered_set<char>{'x', 'x', 'x'}); }
TEST(SerDes, IntToCharUnorderedMap) { test(std::unordered_map<int, char>{{1, 'a'}, {2, 'b'}, {3, 'c'}, {4, 'a'}}); }
TEST(SerDes, CharToIntUnorderedMultimap) { test(std::unordered_multimap<char, int>{{'a', 1}, {'b', 2}, {'c', 3}, {'a', 4}}); }

TEST(SerDes, MultilevelDocumentStructure) {
    test(std::tuple<std::set<std::string>, std::map<std::pair<std::string, std::string>, int>, std::list<std::vector<std::string>>> (
        {"alpha", "beta", "delta", "gamma", "epsilon"},
        {
            {{"alpha", "beta"}, 1},
            {{"beta", "gamma"}, 2},
            {{"alpha", "delta"}, 3},
            {{"delta", "gamma"}, 4},
            {{"gamma", "epsilon"}, 5}
        },
        {
            {"alpha", "beta", "gamma", "beta", "alpha"},
            {"alpha", "delta", "gamma", "epsilon"}
        }
    ));
}

struct CustomDataRw
{
    int x = 0;
    CustomDataRw() = default;
    CustomDataRw(int x): x(x) {}
    bool operator ==(const CustomDataRw&o) const { return x == o.x; }
};

struct CustomDataRo
{
    unsigned long long x = 0;
    CustomDataRo() = default;
    CustomDataRo(unsigned long long x): x(x) {}
    bool operator ==(const CustomDataRo&o) const { return x == o.x; }
};

struct CustomDataWo
{
    unsigned long long x = 0;
    CustomDataWo() = default;
    CustomDataWo(unsigned long long x): x(x) {}
    bool operator ==(const CustomDataWo&o) const { return x == o.x; }
};

namespace rpc {

template<> struct TypeInfo<CustomDataRo>
{
    template<class S> static constexpr inline bool read(S &s, CustomDataRo& v)  { return s.read(v.x); }
};

template<> struct TypeInfo<CustomDataWo>
{
    static constexpr inline size_t size(const CustomDataWo& v) { return sizeof(v.x); }
    static constexpr inline bool isConstSize() { return true; }
    template<class S> static constexpr inline bool write(S &s, const CustomDataWo& v) { return s.write(v.x); }
};

template<> struct TypeInfo<CustomDataRw>
{
    static constexpr inline size_t size(const CustomDataRw& v) {
        return sizeof(int);
    }

    template<class S> static constexpr inline bool write(S &s, const CustomDataRw& v) {
        return s.write(~v.x);
    }

    template<class S> static constexpr inline bool read(S &s, CustomDataRw& v) 
    {
        if(s.read(v.x))
        {
            v.x = ~v.x;
            return true;
        }

        return false;
    }
};

}

TEST(SerDes, CustomDataRw) 
{
    test(CustomDataRw(123));

    MockStream stream(0);
    CHECK(!read(stream, CustomDataRw(123)));
}

TEST(SerDes, CustomDataDissimilarSingleSided) 
{
    auto data(write(CustomDataWo{420}));
    CHECK(read(data, CustomDataRo{420}));
}

TEST(SerDes, CustomSingleSidedNested) 
{
    auto data = write(std::forward_list<CustomDataWo>{1, 4, 1, 4, 2, 1, 3, 5, 6, 2});
    CHECK(read(data, std::deque<CustomDataRo>{1, 4, 1, 4, 2, 1, 3, 5, 6, 2}));
}

TEST(SerDes, ListAsVector) 
{
    auto data = write(std::list<int>{1, 2, 3});
    CHECK(read(data, std::vector<int>{1, 2, 3}));
}

TEST(SerDes, TupleAsPair) 
{
    auto data = write(std::tuple<char, int>('a', 1));
    CHECK(read(data, std::pair<char, int>('a', 1)));
}

TEST(SerDes, StringIntHashMapAsVectorOfPairs) 
{
    auto data = write(std::map<std::string, int>{{"foo", 42}, {"bar", 69}});
    CHECK(read(data, std::vector<std::pair<std::string, int>>{{"bar", 69}, {"foo", 42}}));
}

TEST(SerDes, LongStrings) 
{
    test(std::string(128, '.'));
    test(std::string(128*128, '.'));
    test(std::string(128*128*128, '.'));
    // Takes too long: test(std::string(128*128*128*128, '.'));
}

TEST(SerDes, Truncate) 
{
    for(int i=0; ; i++)
    {
        auto data = write(std::string("abc"));

        if(!data.truncateAt(i))
        {
            CHECK(read(data, std::forward_list<char>{'a', 'b', 'c'}));
            break;
        }

        CHECK(!read(data, std::forward_list<char>{'a', 'b', 'c'}));
    }
}

TEST(SerDes, NoSpace)
{
    auto data = std::string("panzerkampfwagen");
    auto s = rpc::determineSize(data);

    for(auto i = 0u; i <= s; i++)
    {
        MockStream stream(i);
        auto a = stream.access();
        bool sok = rpc::serialize(a, data);

        if(i < s)
            CHECK(!sok);
        else
            CHECK(sok);
    }
}

TEST(SerDes, VarUint4) 
{
    uint32_t ns[] = {0, 1, 127, 128, 129, 
        128*128-1, 128*128, 128*128+1, 
        128*128*128-1, 128*128*128, 128*128*128+1, 
        128*128*128*128-1, 128*128*128*128, 128*128*128*128+1};

    for(auto n: ns)
    {
        const auto s = rpc::VarUint4::size(n);

        for(auto i = 0u; i < s; i++)
        {
            MockStream stream(i);
            auto a = stream.access();
            CHECK(!rpc::VarUint4::write(a, n));
        }

        MockStream stream(s);
        auto a = stream.access();
        CHECK(rpc::VarUint4::write(a, n));
        CHECK(!a.write('\0'));

        uint32_t r;
        auto b = stream.access();
        CHECK(rpc::VarUint4::read(b, r));
        CHECK(r == n);
    }
}

TEST(SerDes, SimpleStreamReader)
{
    auto data = write(std::string("streaming"));
    auto b = data.access();
    CHECK(nullptr == rpc::deserialize<rpc::StreamReader<char, MockStream::Accessor>>(b, [](auto&& reader)
    {
        std::stringstream ss;

        for(char c: reader)
            ss << c;

        CHECK(ss.str() == "streaming");
    }));
}

TEST(SerDes, StreamCall)
{
    auto exp = std::vector<rpc::Call<>>{makeCall<>(5), makeCall<>(3), makeCall<>(1)};
    auto data = write(exp, 123);
    auto b = data.access();

    CHECK(nullptr == rpc::deserialize<rpc::StreamReader<rpc::Call<>, MockStream::Accessor>, int>(b, [&exp](auto&& reader, int n)
    {
        auto e = exp.begin();

        for(const auto &x: reader)
            CHECK(x == *e++);

        CHECK(n == 123);
    }));
}

TEST(SerDes, StreamCollection)
{
    auto exp = std::vector<std::string>{"the", "quick", "brown", "fox", "..."};
    auto data = write(exp, short(321));
    auto b = data.access();

    CHECK(nullptr == rpc::deserialize<rpc::StreamReader<std::string, MockStream::Accessor>, short>(b, [&exp](auto&& reader, short n) 
    {
        auto e = exp.begin();
        for(const auto &x: reader)
            CHECK(x == *e++);

        CHECK(n == 321);
    }));
}

TEST(SerDes, StreamTuple)
{
    auto exp = std::vector<std::tuple<char, int>>{{'a', 1}, {'b', 2}, {'c', 3}};
    auto data = write(exp, 'x');
    auto b = data.access();

    CHECK(nullptr == rpc::deserialize<rpc::StreamReader<std::tuple<char, int>, MockStream::Accessor>, char> (b,
    [&exp](auto&& reader, char c) 
    {
        auto e = exp.begin();
        for(const auto &x: reader)
            CHECK(x == *e++);

        CHECK(c == 'x');
    }));
}

TEST(SerDes, StreamStream)
{
    auto exp = std::vector<std::string>{"sufficiently", "advanced", "technology"};
    auto data = write(exp, long(9876543210l));
    auto b = data.access();

    bool done = false;
    CHECK(nullptr == rpc::deserialize<rpc::StreamReader<rpc::StreamReader<char, MockStream::Accessor>, MockStream::Accessor>, long> (b,
    [&done, &exp](auto&& reader, long l) 
    {
        auto e = exp.begin();
        for(auto x: reader)
        {
            std::stringstream ss;

            for(char c: x)
                ss << c;

            CHECK(ss.str() == *e++);
            done = true;
        }

        CHECK(e == exp.end());
        CHECK(l == 9876543210l);
    }));

    CHECK(done);
}

TEST(SerDes, StreamClobber)
{
    const auto exp = std::map<std::string, rpc::Call<>>{{"zero", makeCall<>(0)}, {"one", makeCall(1)}, {"many", makeCall(268435456)}};

    for(int i = 0; ; i++)
    {
        auto data = write(exp, (bool)((i & 1) == 0));

        bool truncated = data.truncateAt(i);
        auto b = data.access();
        bool done = false;

        bool executed = nullptr == rpc::deserialize<rpc::StreamReader<std::pair<std::string, rpc::Call<>>, MockStream::Accessor>, bool>(b, 
        [&exp, &done, i](auto&& reader, bool f)
        {
            auto elements = exp;
            for(const auto &x: reader)
            {
                auto it = elements.find(x.first);
                CHECK(it != elements.end());
                CHECK(it->second == x.second);
                elements.erase(it);
            }

            CHECK(elements.size() == 0);
            CHECK(f == ((i & 1) == 0));
            done = true;
        });

        if(truncated)
            CHECK(!executed && !done);
        else
        {
            CHECK(executed && done);
            break;
        }
    }
}

TEST(SerDes, ArrayWriter)
{
    unsigned short exp[] = {0xb16b, 0x00b5};
    rpc::ArrayWriter<unsigned short> input(exp);

    auto size = rpc::determineSize(input);
    for(auto i = 0u; i < size; i++)
    {
        MockStream stream(i);
        auto a = stream.access();
        CHECK(!rpc::serialize(a, input));
    }

    for(int i = 0; ; i++)
    {
        auto data = write(input, -i);

        bool truncated = data.truncateAt(i);
        auto b = data.access();
        bool done = false;

        bool executed = nullptr == rpc::deserialize<std::vector<unsigned short>, int>(b, 
        [exp, i, &done](const auto& data, int j)
        {
            CHECK(data == std::vector<unsigned short>(exp, exp + sizeof(exp)/sizeof(exp[0])));
            CHECK(i == -j);
            done = true;
        });

        if(truncated)
            CHECK(!executed && !done);
        else
        {
            CHECK(executed && done);
            break;
        }
    }
}

TEST(SerDes, CTStr)
{
    static constexpr const char str[] = "indistinguishable";
    static constexpr auto ctstr = rpc::CTStr<sizeof(str)-1>(str);
    auto data = write(rpc::ArrayWriter<char>((const char*)ctstr, ctstr.strLength));
    CHECK(read(data, std::string(str)));
}

TEST(SerDes, StreamOverread)
{
    auto data = write(std::string("frob"));
    auto b = data.access();
    CHECK(nullptr == rpc::deserialize<rpc::StreamReader<char, MockStream::Accessor>>(b, [](auto&& reader)
    {
        std::stringstream ss;

        auto cursor = reader.begin();
        for(auto i = 0u; i < reader.size(); i++)
        {
            char c;
            CHECK(cursor.read(c));
            ss << c;
        }

        CHECK(ss.str() == "frob");
        
        for(int i = 0; i < 100; i++)
        {
            char _;
            CHECK(!cursor.read(_));
            _ = *cursor;
        }
    }));
}

TEST(SerDes, ExtraArgs)
{
    auto data = write(std::string("normal"));
    auto b = data.access();
    bool done = false;

    CHECK(nullptr == rpc::deserialize<std::string>(b, [&done](auto extra, auto normal)
    {
        CHECK(normal == "normal");
        CHECK(extra == "extra");
        done = true;
    }, std::string("extra")));

    CHECK(done);
}

TEST(SerDes, CollectionGenerator)
{
    unsigned short exp[] = {0xb16b, 0x00b5};
    auto input = rpc::generateCollection(2, [&exp, i(0u)]() mutable
	{
    	CHECK(i < sizeof(exp)/sizeof(exp[0]));
    	return exp[i++];
    });

    auto size = rpc::determineSize(input);
    for(auto i = 0u; i < size; i++)
    {
        MockStream stream(i);
        auto a = stream.access();
        CHECK(!rpc::serialize(a, input));
    }

    bool done = false;
	auto data = write(123, input, 456);
	auto b = data.access();
	auto res = rpc::deserialize<int, std::vector<unsigned short>, int>(b,
        [exp, &done](int a, const auto& data, int b)
        {
			CHECK(a == 123);
            CHECK(data == std::vector<unsigned short>(exp, exp + sizeof(exp)/sizeof(exp[0])));
            CHECK(b == 456);
            done = true;
        }
	);

	CHECK(res == nullptr);
	CHECK(done);
}

struct StructTest
{
	int n;
	std::string str;

	inline bool operator ==(const StructTest& o) const {
		return n == o.n && str == o.str;
	}
};

namespace rpc {

template<> struct TypeInfo<StructTest>: StructTypeInfo<StructTest,
		StructMember<&StructTest::n>,
		StructMember<&StructTest::str>
	>{};
}

TEST(SerDes, Struct)
{
    test(StructTest{1337, "leet"});
}

struct NestedStructTest
{
	StructTest sa, sb;
	std::string explanation;

	inline bool operator ==(const NestedStructTest& o) const {
		return sa == o.sa && sb == o.sb && explanation == o.explanation;
	}
};

namespace rpc {

template<> struct TypeInfo<NestedStructTest>: StructTypeInfo<NestedStructTest,
		StructMember<&NestedStructTest::sa>,
		StructMember<&NestedStructTest::sb>,
		StructMember<&NestedStructTest::explanation>
	>{};
}

TEST(SerDes, NestedStructs)
{
    test(NestedStructTest
	{
		{0x1337, "l33t"},
		{1337, "1ee7"},
		"foobar"
	});
}

