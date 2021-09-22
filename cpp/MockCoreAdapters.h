#ifndef _MOCKCOREADAPTERS_H_
#define _MOCKCOREADAPTERS_H_

#include "MockStream.h"
#include "1test/Test.h"

#include <map>

class MockStreamWriterFactory;

struct SelfContainedStreamWriter: MockStream, MockStream::Accessor
{
	MockStreamWriterFactory& f;
    inline SelfContainedStreamWriter(MockStreamWriterFactory& f, size_t s);
    inline ~SelfContainedStreamWriter();
};

struct MockStreamWriterFactory
{
    using Accessor = MockStream::Accessor;
    int inUse = 0;

    static inline int failAt = 0;

    inline auto build(size_t s)
    {
        if(failAt > 0)
        {
            if(--failAt == 0)
                return SelfContainedStreamWriter(*this, 0);
        }

        return SelfContainedStreamWriter(*this, s);
    }

    decltype(auto) inline done(SelfContainedStreamWriter &&w) {
        return static_cast<MockStream&&>(w); 
    }

    inline ~MockStreamWriterFactory() {
    	CHECK(inUse == 0);
    }
};

inline SelfContainedStreamWriter::SelfContainedStreamWriter(MockStreamWriterFactory& f, size_t s): MockStream(s), MockStream::Accessor(this->access()), f(f) {
	f.inUse++;
}

SelfContainedStreamWriter::~SelfContainedStreamWriter() {
	f.inUse--;
}

template<class K, class V>
class MockRegistry
{
    std::map<K, V> lookupTable;

public:
    inline bool remove(const K& k) 
    {
        auto it = lookupTable.find(k);

        if(it == lookupTable.end())
            return false;

        lookupTable.erase(it);
        return true;
    }

    inline bool add(const K& k, V&& v) 
    {
        auto it = lookupTable.find(k);

        if(it != lookupTable.end())
            return false;

        return lookupTable.emplace(k, std::move(v)).second;
    }

    inline V* find(const K& k, bool &ok) 
    {
        auto it = lookupTable.find(k);

        if(it == lookupTable.end())
        {
            ok = false;
            return nullptr;
        }
        
        ok = true;
        return &it->second;
    }
};

template<class T>
struct MockSmartPointer: std::unique_ptr<T> 
{
    MockSmartPointer(std::unique_ptr<T> &&v): std::unique_ptr<T>(std::move(v)) {}

    template<class U, class... Args>
    static inline MockSmartPointer make(Args&&... args) {
        return MockSmartPointer(std::unique_ptr<T>(new U(std::forward<Args>(args)...)));
    }
};


#endif /* _MOCKCOREADAPTERS_H_ */
