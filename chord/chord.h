#pragma once

#include <argp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <openssl/sha.h>
#include <time.h>
#include <deque>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "proto/chord.pb.h"

#define MAX_TCP_CONNECTIONS 10
