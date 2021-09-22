#ifndef RPC_CPP_INTEROP_COMMON_H_
#define RPC_CPP_INTEROP_COMMON_H_

#include <iostream>
#include <thread>

static constexpr uint16_t defaultInitial = 2;
static constexpr uint16_t defaultModulus = 19;

template<class Target>
static inline auto startServiceThread(std::shared_ptr<Target> uut)
{
    return std::thread([uut]()
	{
        while(((rpc::FdStreamAdapter*)uut.get())->receive([&uut](auto message)
        {
            if(auto a = message.access(); auto err = uut->process(a))
                std::cerr << "RPC SRV: " << err << std::endl;

            return true;
        }));
    });
}

#endif /* RPC_CPP_INTEROP_COMMON_H_ */
