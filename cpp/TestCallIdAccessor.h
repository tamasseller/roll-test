#ifndef _TESTCALLIDACCESSOR_H_
#define _TESTCALLIDACCESSOR_H_

#include "RpcCall.h"

namespace rpc
{
    struct CallIdTestAccessor {
        template<class... Args>
        static inline auto makeCall(uint32_t id) {
            return rpc::Call<Args...>(id);
        }

        template<class... Args>
        static inline auto getId(const rpc::Call<Args...>&c) {
            return c.id;
        }
    };
}

#endif /* _TESTCALLIDACCESSOR_H_ */
