# korus (means chorus)
##  before build, please:
#### 1, search file "shared_ptr_base.h", cmd just as: "find /usr/ -name shared_ptr.h"
#### 2, edit "shared_ptr_base.h", add code just below in public scope of template class "__shared_ptr":
####        //korus 20180107 for manual ref 
####        void inref() noexcept
####        {       _M_refcount._M_add_ref_lock();  }
####        //korus 20180107 for manual ref 
####        void deref() noexcept
####        {       _M_refcount._M_release();      }
        
##  now feb 2018, only demo, without strength test yet

####  development plan：      
#####    feb 2018 support quic，websocket, async mysql/redis client.      
#####    mar 2018 support http(s) client/server, http-proxy.          
#####    apr 2018 support log, report api and falcon, yaml conf.      
#####    may 2018 support multi-paxos, load-balance client/server.      
