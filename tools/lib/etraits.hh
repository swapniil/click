// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ETRAITS_HH
#define CLICK_ETRAITS_HH

struct Driver {
    enum {
	USERLEVEL = 0, LINUXMODULE = 1, BSDMODULE = 2, NSMODULE = 3,
	ALLMASK = 0xF, COUNT = 4
    };
    static const char *name(int);
    static const char *requirement(int);
};
    

struct ElementTraits {
    
    String name;
    String cxx;
    String documentation_name;
    String header_file;
    String source_file;
    String flags;
    String requirements;
    String provisions;
    int def_index;
    int driver_mask;
    int name_next;

    ElementTraits();

    static const ElementTraits &null_traits()	{ return the_null_traits; }
    
    bool allows_driver(int d) const	{ return (driver_mask&(1<<d)) != 0; }

    const String &processing_code() const	{ return _processing_code; }
    const String &flow_code() const		{ return _flow_code; }
    bool requires(const String &) const;
    bool provides(const String &) const;
    int flag_value(int) const;
    
    String *component(int);
    String *component(const String &);

    void calculate_driver_mask();

    enum {
	D_NONE,
	D_CLASS, D_CXX_CLASS, D_HEADER_FILE, D_PROCESSING,
	D_FLOW_CODE, D_FLAGS, D_REQUIREMENTS, D_PROVISIONS,
	D_SOURCE_FILE, D_DOC_NAME,
	D_FIRST_DEFAULT = D_CLASS, D_LAST_DEFAULT = D_PROVISIONS
    };
    static int parse_component(const String &);
    static ElementTraits make(int, ...);

  private:
    
    String _processing_code;
    String _flow_code;

    static ElementTraits the_null_traits;

    friend class ElementMap;
    
};

typedef ElementTraits Traits;


inline
ElementTraits::ElementTraits()
    : def_index(0), driver_mask(Driver::ALLMASK), name_next(0)
{
}

inline String *
ElementTraits::component(const String &s)
{
    return component(parse_component(s));
}

#endif
