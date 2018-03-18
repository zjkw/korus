# korus (means chorus)
##  before build, please:
#### 1, search file "shared_ptr_base.h", cmd just as: "find /usr/ -name shared_ptr.h"
#### 2, edit "shared_ptr_base.h"。
#####         A，add code just below in public scope of template class "__shared_ptr":
######        //korus 20180107 for manual ref 
######        void inref() noexcept
######        { _M_refcount._M_add_ref_lock();  }
######        void deref() noexcept
######        { _M_refcount._M_release();      }
#####         B， add code just below in public scope of template class "__shared_count":
######        //korus 20180107 for manual ref      
######        void _M_add_ref_lock() const noexcept
######        { if(_M_pi) _M_pi->_M_add_ref_lock();     }
######        void _M_release() const noexcept
######        { if(_M_pi) _M_pi->_M_release();  }
        
##  now feb 2018, only demo, without strength test yet

####  development plan：      
#####    mar 2018 support quic，websocket, async mysql/redis client.      
#####    apr 2018 support http(s) client/server, http-proxy.          
#####    may 2018 support log, report api and falcon, yaml conf.       
#####    jun 2018 support multi-paxos, load-balance client/server.      
