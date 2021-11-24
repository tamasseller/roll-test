#ifndef RPC_CPP_INTEROP_COMMON_H_
#define RPC_CPP_INTEROP_COMMON_H_

#include <iostream>
#include <thread>

static constexpr uint16_t defaultInitial = 2;
static constexpr uint16_t defaultModulus = 19;

template<class Target>
static inline auto startLooper(std::shared_ptr<Target> uut)
{
    return std::thread([uut]()
	{
        while(((rpc::FdStreamAdapter*)uut.get())->receive([&uut](auto message)
        {
            if(auto a = message.access(), err = uut->process(a); !!err)
			{
            	auto action = rpc::getExpectedExecutorBehavior(err);

            	if(action == rpc::ExpectedExecutorBehavior::Log)
				{
            		std::cerr << "RPC SRV: " << rpc::getErrorString(err) << std::endl;
				}
            	else if(action == rpc::ExpectedExecutorBehavior::Drop)
				{
            		std::cerr << "Dropping connection due to: " << rpc::getErrorString(err) << std::endl;
            		abort();
				}
            	else if(action == rpc::ExpectedExecutorBehavior::Die)
				{
            		std::cerr << "Fatal error occurred: " << rpc::getErrorString(err) << std::endl;
            		abort();
				}
			}

            return true;
        }));

        uut->connectionClosed();
    });
}


#endif /* RPC_CPP_INTEROP_COMMON_H_ */
