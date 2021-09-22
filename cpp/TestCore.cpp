#include "RpcCore.h"
#include "RpcStlArray.h"
#include "RpcStlList.h"

#include "MockCoreAdapters.h"

#include "1test/Test.h"

TEST_GROUP(Core)  {};

TEST(Core, AddCallAt)
{
    rpc::Core<MockStream::Accessor, MockSmartPointer, MockRegistry> core;
    bool buildOk;
    MockStreamWriterFactory f1;
    auto call = core.buildCall<std::string>(f1, buildOk, 69, std::string("asdqwe"));
    CHECK(buildOk);

    bool done = false;

    auto a = call.access();
    CHECK(core.execute(a) == rpc::Errors::wrongMethodRequest);
    CHECK(!done);

    CHECK(core.addCallAt<std::string>(69, [&done](const rpc::MethodHandle &id, std::string str)
    {
        CHECK(str == "asdqwe");
        done = true;
    }));

    CHECK(!done);

    auto b = call.access();
    CHECK(nullptr == core.execute(b));
    CHECK(done);

    done = false;
    auto c = call.access();
    CHECK(nullptr == core.execute(c));
    CHECK(done);

    CHECK(!core.addCallAt<std::string>(69, [&done](const rpc::MethodHandle &id, std::string str) { CHECK(false); }));

    done = false;
    auto d = call.access();
    CHECK(nullptr == core.execute(d));
    CHECK(done);

    CHECK(core.removeCall(69));

    done = false;
    auto e = call.access();
    CHECK(core.execute(e) == rpc::Errors::wrongMethodRequest);
    CHECK(!done);

    CHECK(!core.removeCall(69));
}

TEST(Core, Truncate)
{
    rpc::Core<MockStream::Accessor, MockSmartPointer, MockRegistry> core;

    bool done = false;
    CHECK(core.addCallAt<std::list<std::vector<char>>>(69, [&done](const rpc::MethodHandle &id, auto str)
    {
        CHECK(str == std::list<std::vector<char>>{{'a', 's', 'd'}, {'q', 'w', 'e'}});
        CHECK(!done);
        done = true;
    }));

    for(int i = 0; ; i++)
    {
        bool buildOk;
        MockStreamWriterFactory f2;
        auto call = core.buildCall<std::vector<std::string>>(f2, buildOk, 69, std::vector<std::string>{"asd", "qwe"});
        CHECK(buildOk);

        if(call.truncateAt(i))
        {
            auto a = call.access();
            CHECK(core.execute(a) == rpc::Errors::messageFormatError);
        }
        else
        {
            auto a = call.access();
            CHECK(nullptr == core.execute(a));
            break;
        }
    }

    CHECK(done);
}

TEST(Core, GenericInsert)
{
    rpc::Core<MockStream::Accessor, MockSmartPointer, MockRegistry> core;

    bool a = false;
    int b = 0;

    core.addCallAt(0, [](const rpc::MethodHandle &id){});
    auto id1 = core.add([&a](auto id) { a = true; });
    auto id2 = core.add<int, int>([&b](const rpc::MethodHandle &id, int x, int y) { b = x + y; });

    bool build1ok;
    MockStreamWriterFactory f1;
    auto call1 = core.buildCall(f1, build1ok, id1);
    CHECK(build1ok);

    auto r1 = call1.access();
    CHECK(nullptr == core.execute(r1));
    CHECK(a == true);

    bool build2ok;
    MockStreamWriterFactory f2;
    auto call2 = core.buildCall<int, int>(f2, build2ok, id2, 1, 2);
    CHECK(build2ok);
    auto r2 = call2.access();
    CHECK(nullptr == core.execute(r2));
    CHECK(b == 3);
}
