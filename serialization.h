#ifndef SERIALIZATION_H
#define SERIALIZATION_H 1

// Some convenience stuff for serialization

// Note: The order is important on some compilers. All boost/serialization
// includes should go here! (except boost/archive, which should be placed
// before this header in the .cpp file ... I think)

#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>

#define SER_PROTO(friend, fname, c, const) \
    template <class Archive> friend void fname(Archive &ar, const c & obj, const unsigned int version)

// put this in serializable classes, to befriend any serializers in use
#define FRIEND_SERIALIZE(c) \
    friend class boost::serialization::access; \
    SER_PROTO(friend, o_save, c, const); \
    SER_PROTO(friend, o_load, c, ); \
    SER_PROTO(friend, o_serialize, c, );


#define WRAP_SPLIT(c) \
    BOOST_SERIALIZATION_SPLIT_FREE(c); \
    BOOST_CLASS_EXPORT(c); \
    namespace boost { namespace serialization { \
        SER_PROTO(inline, load, c,) { \
            o_load(ar, obj, version); \
        } \
        SER_PROTO(inline, save, c, const) { \
            o_save(ar, obj, version); \
        } \
    } }

#define WRAP_SERIALIZE(c) \
    BOOST_CLASS_EXPORT(c); \
    namespace boost { namespace serialization { \
        SER_PROTO(inline, serialize, c,) { \
            o_serialize(ar, obj, version); \
        } \
    } }
    
#define SER_BASE(ar,bc) \
    do { ar & boost::serialization::base_object<bc>(*this); } while (0)

#define SAVE(c) WRAP_SPLIT(c); SER_PROTO(, o_save, c, const)
#define LOAD(c) SER_PROTO(, o_load, c,)
#define SERIALIZE(c) WRAP_SERIALIZE(c); SER_PROTO(, o_serialize, c,)

#endif
