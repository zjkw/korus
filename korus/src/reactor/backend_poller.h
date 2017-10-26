#pragma once

#include "korus/src/util/basic_defines.h"
#include "sockio_helper.h"

class backend_poller
{
public:
	backend_poller();
	virtual ~backend_poller();

	virtual int32_t poll(int32_t mill_sec) = 0;
	// 初始新增
	virtual void add_sock(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type) = 0;
	// 存在更新
	virtual void upt_type(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type) = 0;
	// 删除
	virtual void del_sock(sockio_helper* sockio_id, SOCKET fd) = 0;
	virtual void clear() = 0;
};

