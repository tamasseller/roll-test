#include "types/CallTypeInfo.h"
#include "types/ArrayTypeInfo.h"
#include "types/PrimitiveTypeInfo.h"
#include "types/StdStringTypeInfo.h"

#include "base/RpcEndpoint.h"

#include "MockCoreAdapters.h"
#include "TestCallIdAccessor.h"

#include "1test/Test.h"

#include <list>
#include <sstream>

static constexpr const char hello[] = {'h', 'e', 'l', 'l', 'o'};

struct Uut:
		rpc::Endpoint<
			MockSmartPointer,
			MockRegistry,
			MockStreamWriterFactory::Accessor,
			Uut
		>
{
    std::list<MockStream> sent;

    int failAt = 0;

    inline bool send(MockStream &&s) 
    {
        if(failAt > 0)
        {
            if(--failAt == 0)
                return false;
        }
        
        sent.emplace_back(std::move(s));
        return true;
    }

    inline auto messageFactory() {
    	return MockStreamWriterFactory{};
    }
};

TEST_GROUP(Endpoint), rpc::CallIdTestAccessor
{
    static inline void executeLoopback(Uut::Endpoint& ep, rpc::Errors exp = rpc::Errors::success)
    {
        auto &uut = (Uut&)ep;
        auto msg = rpc::move(uut.sent.front());
        uut.sent.pop_front();
        auto loopback = msg.access();
        CHECK(exp == uut.process(loopback));
    }
};

TEST(Endpoint, Hello)
{
    Uut uut;
    CHECK(uut.init());

    int n = 0;
    rpc::Call<std::string> cb = uut.install([&n](Uut::Endpoint& uut, const rpc::MethodHandle &h, const std::string &str) {
        CHECK(str == "hello");
        n++;
    });

    CHECK(rpc::Errors::success == uut.call(cb, hello));
    executeLoopback(uut);
    CHECK(n == 1);
}

TEST(Endpoint, NotHello)
{
    Uut uut;
    CHECK(uut.init());
    rpc::Call<std::string> cb = uut.install([](Uut::Endpoint& uut, const rpc::MethodHandle &h, const std::string &str) {});
    CHECK(rpc::Errors::success == uut.uninstall(cb));
    CHECK(uut.uninstall(cb) == rpc::Errors::methodNotFound);
}

TEST(Endpoint, ProvideRequire)
{
    Uut uut;
    CHECK(uut.init());

    constexpr auto sym = rpc::symbol<uint32_t, rpc::Call<std::string>>("symbol"_ctstr);

    CHECK(rpc::Errors::success == uut.provide(sym, [](Uut::Endpoint& uut, const rpc::MethodHandle &id, uint32_t x, rpc::Call<std::string> callback) {}));

    bool done = false;
    CHECK(rpc::Errors::success == uut.lookup(sym, [&done](Uut::Endpoint& uut, bool lookupSucced, auto sayHello)
    {
        CHECK(lookupSucced);

        (rpc::Call<uint32_t, rpc::Call<std::string>>)sayHello;
        CHECK(getId(sayHello) == 1);
        done = true;
    }));

    for(int i = 0; i < 2; i++)
        executeLoopback(uut);

    CHECK(done);
}

TEST(Endpoint, ProvideDiscardRequire)
{
    Uut uut;
    CHECK(uut.init());

    constexpr auto sym = rpc::symbol<uint32_t, rpc::Call<std::string>>("symbol"_ctstr);
    CHECK(uut.discard(sym) == rpc::Errors::symbolNotFound);

    CHECK(rpc::Errors::success == uut.provide(sym, [](Uut::Endpoint& uut, const rpc::MethodHandle &id, uint32_t x, rpc::Call<std::string> callback) {}));
    CHECK(rpc::Errors::success == uut.discard(sym));

    bool done = false;
    CHECK(rpc::Errors::success == uut.lookup(sym, [&done](Uut::Endpoint& uut, bool lookupSucced, auto)
    {
        CHECK(!lookupSucced);
        done = true;
    }));

    executeLoopback(uut, rpc::Errors::unknownSymbolRequested);
    executeLoopback(uut);

    CHECK(uut.discard(sym) == rpc::Errors::symbolNotFound);

    CHECK(done);
}

TEST(Endpoint, DoubleProvide)
{
    Uut uut;
    CHECK(uut.init());

    constexpr auto sym = rpc::symbol<long>("you-only-provide-once"_ctstr);
    CHECK(rpc::Errors::success == uut.provide(sym, [](Uut::Endpoint& uut, rpc::MethodHandle id, long x){}));
    CHECK(rpc::Errors::symbolAlreadyExported == uut.provide(sym, [](Uut::Endpoint& uut, rpc::MethodHandle id, long x){}));
}

TEST(Endpoint, DoubleInit)
{
    Uut uut;
    CHECK(uut.init());
    CHECK(!uut.init());
}

TEST(Endpoint, ExecuteRemoteWithCallback)
{
    Uut uut;
    CHECK(uut.init());

    constexpr auto sym = rpc::symbol<uint32_t, rpc::Call<std::string>>("say-hello"_ctstr);

    CHECK(rpc::Errors::success == uut.provide(sym, [](Uut::Endpoint& uut, const rpc::MethodHandle &id, uint32_t x, rpc::Call<std::string> callback)
	{
        for(auto i = 0u; i < x; i++)
            CHECK(rpc::Errors::success == uut.call(callback, hello));
    }));

    bool done = false;
    CHECK(rpc::Errors::success == uut.lookup(sym, [&done](Uut::Endpoint& uut, bool lookupSucceded, auto sayHello)
    {
        CHECK(lookupSucceded);

        int n = 0;
        rpc::Call<std::string> cb = uut.install([&n](Uut::Endpoint &uut, const rpc::MethodHandle &id, const std::string &str)
		{
            CHECK(str == "hello");
            n++;
        });
    
        CHECK(rpc::Errors::success == uut.call(sayHello, 3u, cb));

        for(int i = 0; i < 4; i++)
            executeLoopback(uut);

        CHECK(n == 3);
        done = true;
    }));

    for(int i=0; i<2; i++)
        executeLoopback(uut);

    CHECK(done);
}

TEST(Endpoint, LookupTotallyUnknownMethod)
{
    Uut uut;
    CHECK(uut.init());

    constexpr auto sym = rpc::symbol<uint32_t>("non-existent"_ctstr);

    bool done = false;
    CHECK(rpc::Errors::success == uut.lookup(sym, [&done](auto &uut, bool lookupSucceded, auto)
    {
        CHECK(!lookupSucceded);
        done = true;
    }));  

    executeLoopback(uut, rpc::Errors::unknownSymbolRequested);
    executeLoopback(uut);

    CHECK(done);
}

TEST(Endpoint, LookupMethodWithMistmatchedSignatureUnknownMethod)
{
    Uut uut;
    CHECK(uut.init());

    constexpr auto defsym = rpc::symbol<std::string>("almost"_ctstr);

    CHECK(rpc::Errors::success == uut.provide(defsym, [](Uut::Endpoint& uut, const rpc::MethodHandle &id, std::string callback){}));

    constexpr auto lookupsym = rpc::symbol<int>("almost"_ctstr);

    bool done = false;
    CHECK(rpc::Errors::success == uut.lookup(lookupsym, [&done](auto &uut, bool lookupSucceded, auto)
    {
        CHECK(!lookupSucceded);
        done = true;
    }));  

    executeLoopback(uut, rpc::Errors::unknownSymbolRequested);
    executeLoopback(uut);

    CHECK(done);
}

TEST(Endpoint, FailToSendLookup)
{
    Uut uut;
    CHECK(uut.init());
    bool done = false;

    constexpr auto sym = rpc::symbol<>("dont-care"_ctstr);

    uut.failAt = 1;

    CHECK(uut.lookup(sym, [&done](auto &uut, bool lookupSucceded, auto sayHello){ done = true; }) 
        == rpc::Errors::couldNotSendLookupMessage);

    CHECK(!done);
}

TEST(Endpoint, FailToSendLookupResponse)
{
    Uut uut;
    CHECK(uut.init());
    bool done = false;

    constexpr auto sym = rpc::symbol<>("dont-care"_ctstr);

    uut.failAt = 2;

    CHECK(rpc::Errors::success == uut.lookup(sym, [&done](auto &uut, bool lookupSucceded, auto sayHello){ done = true; }));

    executeLoopback(uut, rpc::Errors::couldNotSendMessage);

    CHECK(!done);
}

TEST(Endpoint, FailToCreateLookup)
{
    Uut uut;
    CHECK(uut.init());
    bool done = false;

    constexpr auto sym = rpc::symbol<>("dont-care"_ctstr);

    MockStreamWriterFactory::failAt = 1;

    CHECK(uut.lookup(sym, [&done](auto &uut, bool lookupSucceded, auto sayHello){ done = true; })
        == rpc::Errors::couldNotCreateLookupMessage);  

    CHECK(!done);
}

TEST(Endpoint, FailToCreateLookupResponse)
{
    Uut uut;
    CHECK(uut.init());
    bool done = false;

    constexpr auto sym = rpc::symbol<>("dont-care"_ctstr);

    MockStreamWriterFactory::failAt = 2;

    CHECK(rpc::Errors::success == uut.lookup(sym, [&done](auto &uut, bool lookupSucceded, auto sayHello){ done = true; }));
    executeLoopback(uut, rpc::Errors::couldNotCreateMessage);

    CHECK(!done);
}
