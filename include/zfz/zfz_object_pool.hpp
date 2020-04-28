/*********************************************************** 
* Date: 2016-07-08 
* 
* Author: 牟韵 
* 
* Email: mouyun1115@163.com 
* 
* Module: 对象池 
* 
* Brief: 
* 
* Note: 
* 
* CodePage: UTF-8 
************************************************************/ 

/****************************************************************************************************** 
这个文件实现了ObjectPool<T>和ObjectPoolProxy<T>，简化std::shared_ptr<T>的使用以及对象的复用，下面说明如何使用: 

现在我们假设有一个类A（一般应该是纯数据类），在某个函数中我们需要用A的对象实现业务逻辑，那么应该这样使用： 

#include "zfz_object_pool.hpp" //在函数所在的源文件中包含这个头文件 

using namespace zfz; //使用空间 

... function(...)
{
    ...// 其他代码 

    auto a = ObjectPoolProxy<A>::pop_sp(); //直接使用，不需要任何显式的声明或定义 
    用a实现业务逻辑，不用关心它什么时间以及怎样释放 
    如果想把a传递到其他函数则需要清楚std::shared_ptr<T>的特性 

    // 像上面这样使用了pop_sp()来生成对象,那么a的类型是std::shared_ptr<A>， 
    // 如果使用pop()，那么a的类型是A*,且这样生成的a在用完后需要调用push()还回去 
    // 多数场景下建议使用pop_sp() 

    ...//其他代码 
}

如果在此函数的其他位置或任何其他函数中再次使用了ObjectPoolProxy<A>::pop_sp()， 
这样的对象池还有一个特性，那就是所有的这些使用仅会产生一个A的对象池，大家共用。 

此外，建议在类A中实现一个清理所有成员数据的函数，格式限定为 void clear()，比如： 

class A
{
    ...//其他代码 
public://必须是public 
    void clear()
    {
        data1 = 0; //data1的类型应该是整型 
        memset(p, 0, size); //p应该指向一个大小为size字节的缓存 
        data2.clear(); //data2的类型应该是std::vector, std::string或者自定义类型，只要它有void clear()方法 
        ...//其他清理成员数据的代码 
    }
};

即使A不提供void clear()方法,也可以使用ObjectPoolProxy<A>， 
对象池已通过模板元编程在编译期判定这个方法是否存在，存在则调用，不存在则忽略。

如果使类A派生自ObjectPoolProxy，像这样：
class A : public zfz::ObjectPoolProxy<A>
{
    ...
};

那么可以直接这样使用“auto a = A::pop_sp();“， 如此更加简洁，而且ObjectPoolProxy<A>::pop_sp()依旧可以使用， 
二者等效，我们可以自由选择。 

*************************************************************************************************************/
#ifndef	__ZFZ_OBJECT_POOL_HPP_BY_MOUYUN_2014_10_27__
#define	__ZFZ_OBJECT_POOL_HPP_BY_MOUYUN_2014_10_27__

#include <cstdlib>
#include <type_traits>
#include <list>
#include <mutex>
#include <memory>
#include "zfz_sfinae.hpp"
#include "zfz_event.hpp"

namespace zfz
{

template<typename T>
class ObjectPool
{
public:
    ObjectPool()
    {
        available_event_.set();
    }

    ~ObjectPool()
    {
        reset();
    }

private:
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

private:
    std::list<T*> object_list_;
    int max_available_count_ = -1;
    int available_count_ = -1; // 负数代表可用数量不限
    int max_holding_count_ = 8; // when push(), delete or hold, determined by this var, default max holding count is 8 
    std::mutex lock_;
    zfz::Event available_event_;

public:
    T* pop()
    {
        if (available_count_ >= 0)
        {
            while (1)
            {
                available_event_.wait();
                lock_.lock();
                if (available_count_ > 0)
                {
                    break;
                }
                else
                {
                    lock_.unlock();
                }
            }
        }
        else
        {
            lock_.lock();
        }

        T *obj = nullptr;
        if (object_list_.empty())  // if pool is empty, create object 
        {
            obj = new T();
        }
        else
        {
            obj = object_list_.front();
            object_list_.pop_front();
        }

        if (available_count_ >= 0)
        {
            --available_count_;
            if (available_count_ <= 0)
            {
                available_event_.reset();
            }
        }

        lock_.unlock();

        return obj;
    }

    void push(T *obj)
    {
        if (obj == nullptr)
        {
            return;
        }

        std::lock_guard<std::mutex> lg(lock_);

        if (static_cast<int>(object_list_.size()) < max_holding_count_)
        {
            SFINAE::clear_object(obj); /// < using template meta programm 
            object_list_.push_back(obj);
        }
        else
        {
            delete obj; // if pool is full, delete the object 
        }

        if (available_count_ >= 0)
        {
            ++available_count_;
            if (available_count_ == 1)
            {
                available_event_.set();
            }
        }
    }

    void reset()
    {
        lock_.lock();

        int deleted_count = 0;
        auto itr = object_list_.begin();
        while (itr != object_list_.end())
        {
            delete *itr;
            ++deleted_count;
            ++itr;
        }
        object_list_.clear();

        if (available_count_ >= 0)
        {
            available_count_ += deleted_count;
            if (available_count_ == deleted_count && deleted_count > 0)
            {
                available_event_.set();
            }
        }

        lock_.unlock();
    }

    bool set_max_holding_count(const int count)
    {
        if (count < 0)
        {
            return false;
        }

        std::lock_guard<std::mutex> lg(lock_);

        max_holding_count_ = count;

        int deleted_count = 0;
        int current_size = static_cast<int>(object_list_.size()); 
        while (current_size-- > max_holding_count_)
        {
            delete object_list_.front();
            object_list_.pop_front();
            ++deleted_count;
        }

        if (available_count_ >= 0)
        {
            available_count_ += deleted_count;
            if (available_count_ == deleted_count && deleted_count > 0)
            {
                available_event_.set();
            }
        }

        return true;
    }
    inline int get_max_holding_count() const
    {
        return max_holding_count_;
    }
    inline int get_current_holding_count() const
    {
        return static_cast<int>(object_list_.size());
    }

    inline void set_available_count(const int count) // 初始化时设置,运行中不应调用此接口 
    {
        max_available_count_ = count;
        available_count_ = count;
    }
    inline int get_used_count()
    {
        std::lock_guard<std::mutex> lg(lock_);
        return max_available_count_ - available_count_;
    }

    inline int get_available_count() const
    {
        return available_count_;
    }

};

template<typename T>
class ObjectPoolProxy
{
public:
    static inline T* pop()
    {
        return object_pool_.pop();
    }

    static inline void push(T *p)
    {
        object_pool_.push(p);
    }

    static inline void reset()
    {
        object_pool_.reset();
    }

    static inline std::shared_ptr<T> pop_sp()
    {
        return std::shared_ptr<T>(pop(), push);
    }

    static inline bool set_max_holding_count(const int count)
    {
        return object_pool_.set_max_holding_count(count);
    }
    static inline int get_max_holding_count()
    { 
        return object_pool_.get_max_holding_count();
    }
    static inline int get_current_holding_count()
    { 
        return object_pool_.get_current_holding_count();
    }

    static inline void set_available_count(const int count) // 初始化时设置,运行中不应调用此接口 
    {
        object_pool_.set_available_count(count);
    }
    static inline int get_available_count()
    {
        return object_pool_.get_available_count();
    }
    static inline int get_used_count()
    {
        return object_pool_.get_used_count();
    }

private:
    static ObjectPool<T> object_pool_;
};

template<typename T> typename zfz::ObjectPool<T> ObjectPoolProxy<T>::object_pool_; // this is wonderful 

}

#endif
