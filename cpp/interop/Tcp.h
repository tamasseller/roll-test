#ifndef _TCP_H_
#define _TCP_H_

#include <stdint.h>

int listen(uint16_t port);
int connect(const char* addr, uint16_t port);
void closeNow(int);

#endif /* _TCP_H_ */
