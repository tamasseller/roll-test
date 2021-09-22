#include "RpcCTStr.h"

#include "1test/Test.h"

#include <string>

/*
 * Test vectors from:
 *
 * 	 Appendix C of "The FNV Non-Cryptographic Hash Algorithm"
 *   https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17#page-114
 */
static_assert(""_ctstr.hash() == 0xaf63bd4c8601b7dfull);
static_assert("a"_ctstr.hash() == 0x089be207b544f1e4ull);
static_assert("foobar"_ctstr.hash() == 0x34531ca7168b8f38ull);

/*
 * Some plausible signatures.
 */
static_assert("lookup([u8])"_ctstr.hash() == 0xb9d820635b0a84cb);
static_assert("get([i1], ([i1]))"_ctstr.hash() == 0x28ecc5d629430cefull);
static_assert("complicated(u4,(u4,[[i1]]),(()))"_ctstr.hash() == 0xe259accc6c04a22ull);

TEST_GROUP(String) 
{
    struct Worker1 {
        template<class S> static constexpr inline decltype(auto) work(S&& p) {
            return p << "suffix1";
        }
    };

    struct Worker2 {
        template<class S> static constexpr inline decltype(auto) work(S&& p) {
            return p << "suffix2";
        }
    };

    struct NonWorker {
        template<class S> static constexpr inline decltype(auto) work(S&& p) {
            return std::forward<S>(p);
        }
    };

    template<class W, class S> static constexpr inline decltype(auto) append(S&& p) {
        return W::work(std::forward<S>(p));
    }

};

TEST(String, Sanity)
{
    auto asdqwe = "asd"_ctstr << "qwe";
    CHECK(std::string("asdqwe") == (const char*)asdqwe);
}

TEST(String, Constexpr)
{
    constexpr auto asdqwe = "asd"_ctstr << "qwe";
    CHECK(std::string("asdqwe") == (const char*)asdqwe);
}

TEST(String, Delegated)
{
    auto result1 = append<Worker1>("prefix-"_ctstr);
    auto result2 = append<Worker2>("prefix-"_ctstr);
    auto result3 = append<NonWorker>("prefix-"_ctstr);

    char temp[128];

    for(auto i = 0u; i < sizeof(temp) / sizeof(temp[0]); i++)
        temp[i] = i;

    CHECK(std::string("prefix-suffix1") == (const char*)result1);
    CHECK(std::string("prefix-suffix2") == (const char*)result2);
    CHECK(std::string("prefix-") == (const char*)result3);
}
