#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <stdbool.h>

typedef int (*dispatcher_handler_t) (int fd);

bool dispatcher_init(void);
void dispatcher_term(void);

void dispatcher_add(int fd, dispatcher_handler_t rh, dispatcher_handler_t wh);
bool dispatcher_remove(int fd);
int dispatcher_dispatch(void);

#endif // DISPATCHER_HPP
