#include "InteropTest.h"

#include "RpcStlArray.h"
#include "RpcStlList.h"
#include "RpcFdStreamAdapter.h"

#include "Contract.gen.h"

#include "Tcp.h"
#include "Common.h"

#include <random>

using Client = InteropTestContract::ClientProxy<rpc::FdStreamAdapter>;

struct ClientStream: InteropTestStreamClientSession<ClientStream>
{
	uint16_t state = defaultInitial;
	const uint16_t mod = defaultModulus;

	int n = 0;
	bool closed = false;
	std::mutex m;
	std::condition_variable cv;

	ClientStream() = default;
	ClientStream(uint16_t state, uint16_t mod): state(state), mod(mod) {}

	void takeResult(const std::vector<uint16_t> &data)
	{
		for(const auto& v: data)
		{
			assert(v == state);
			state = state * state % mod;
		}

		assert(data.size() == 5);

		{
			std::lock_guard _(m);
			n++;
			cv.notify_all();
		}
	}

	void waitAndClose(std::shared_ptr<Client> uut, int n)
	{
		std::unique_lock<std::mutex> l(m);

		while(this->n != n)
		{
			cv.wait(l);
		}

		this->close(uut);

		while(!this->closed)
		{
			cv.wait(l);
		}
	}

	void onClosed()
	{
		std::lock_guard _(m);
		closed = true;
		cv.notify_all();
	}

	void onOpened() {}
};

class CloseMeSession: public InteropTestInstantCloseClientSession<CloseMeSession>
{
	bool closed = false;
	std::mutex m;
	std::condition_variable cv;

	void wait()
	{
		std::unique_lock<std::mutex> l(m);

		while(!closed)
		{
			cv.wait(l);
		}
	}

public:
	void onClosed()
	{
		std::lock_guard _(m);
		closed = true;
		cv.notify_all();
	}

	void onOpened() {}

	void closeAndWait(std::shared_ptr<Client> uut)
	{
		this->close(uut);
		this->wait();
	}

	void waitAndClose(std::shared_ptr<Client> uut)
	{
		this->wait();
		this->close(uut);
	}
};

static inline void runUnknownMethodLookupTest(std::shared_ptr<Client> uut)
{
    static constexpr auto nope = rpc::symbol<>("nope"_ctstr);

	std::promise<bool> p;
	auto ret = p.get_future();

	const char* err = uut->lookup(nope, [p{std::move(p)}](auto&, bool done, auto) mutable { p.set_value(done); });
	assert(err == nullptr);

	bool failed = ret.get() == false;

    assert(failed);
}

static inline auto generateUniqueKey()
{
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<char> uniform_dist('a', 'z');
    std::stringstream ss;

    for(int i = 0; i < 10; i++)
        ss << uniform_dist(e1);

    return ss.str();
}

static inline void runEchoTest(std::shared_ptr<Client> uut)
{
	bool done = false;

	auto key1 = generateUniqueKey();
	std::vector<char> keyv;
	std::copy(key1.begin(), key1.end(), std::back_inserter(keyv));
    uut->echo(keyv, [&done, key1](const std::string& result)
	{
    	assert(result == key1);
    	done = true;
	});

	auto key2 = generateUniqueKey();
    std::stringstream ss;
    for(char c: uut->echo<std::list<char>>(key2).get())
    {
    	ss.write(&c, 1);
    }

    auto result = ss.str();
    assert(result == key2);

    assert(done);
}

static inline void runStreamGeneratorTest(std::shared_ptr<Client> uut)
{
    auto s = std::make_shared<ClientStream>(3, 31);
    uut->open(s, uint8_t(3), uint8_t(31), [&s, &uut](std::string ret)
	{
    	assert(ret == "asd");

    	for(int i = 0; i < 3; i++)
		{
    		s->generate(uut, uint32_t(5));
		}
    });

    auto t = std::make_shared<ClientStream>();
    uut->openDefault(t, [&t, &uut]() {
   		t->generate(uut, uint32_t(5));
    });

    auto u = std::make_shared<ClientStream>();
    uut->openDefault(u).get();
    u->generate(uut, uint32_t(5));

    auto v = std::make_shared<ClientStream>(5, 17);
    auto str = uut->open<std::string>(v, uint8_t(5), uint8_t(17)).get();
    assert(str == "asd");
    v->generate(uut, uint32_t(5));

    s->waitAndClose(uut, 3);
    t->waitAndClose(uut, 1);
    u->waitAndClose(uut, 1);
    v->waitAndClose(uut, 1);
}


static inline void runRejectionTests(std::shared_ptr<Client> uut)
{
	auto a = std::make_shared<CloseMeSession>();

	uut->closeMe(a, true, [&a](const std::string& str)
	{
		assert(str == "die");
	});

	a->waitAndClose(uut);

	auto b = std::make_shared<CloseMeSession>();
	assert(uut->closeMe<std::string>(b, false).get() == "ok");
	b->closeAndWait(uut);
}

void runInteropTests(int sock)
{
	auto uut = std::make_shared<Client>(sock, sock);
	auto t = startServiceThread(uut);
	uut->unlock(false);

    runUnknownMethodLookupTest(uut);
    runEchoTest(uut);
    runStreamGeneratorTest(uut);
    runRejectionTests(uut);

    uut->unlock(true);

    closeNow(sock);
    t.join();
}

