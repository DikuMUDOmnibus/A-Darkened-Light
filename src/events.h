#define EVENTFUNC(name) long (name)(void *event_obj)


/* function protos need by other modules */
void event_init(void);
struct event *event_create(EVENTFUNC(*func), void *event_obj, long when);
void event_cancel(struct event *event);
void event_process(void);
long event_time(struct event *event);
void event_free_all(void);
int  event_is_queued(struct event *event);

