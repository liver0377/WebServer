#include <iostream>
#include "locker/locker.h"
#include "log/block_queue.h"
#include "log/log.h"
#include <unistd.h>

int main() {
   Log *log = Log::get_instance();
   log->init("web_srv.log", 0, 8192, 500000, 1000);

   sleep(2);

   log->write_log(1, "%s", "test async log writing");

   exit(0);

}