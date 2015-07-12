ssize_t ts_sendmsg(int sd, const struct msghdr *msgh, int flags, struct timespec *ts);

ssize_t ts_recvmsg(int sd,       struct msghdr *msgh, int flags, struct timespec *ts, int *key, int *id);

void ts_print(struct timespec *ts);

int ts_enable_if(const char *dev);

int ts_enable_sd(int sd);

