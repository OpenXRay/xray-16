// This file is excluded from build

struct test_property
{
    int m_property;

    test_property() : m_property(20) {}
    int xr_stdcall getter() { return m_property; }
    void xr_stdcall setter(int value) { m_property = value; }
};

static test_property s_test_property;
static test_property s_test_property_limited;

pcstr s_properties[] = {"integer_property_0", "integer_property_1", "integer_property_2"};

std::pair<int, pcstr> s_properties_enum[] = {std::make_pair(10, "integer_property_0"),
    std::make_pair(20, "integer_property_1"), std::make_pair(30, "integer_property_2")};

static test_property s_test_property_values;
static test_property s_test_property_enum;

struct test_property2
{
    pstr m_property;

    test_property2() : m_property(xr_strdup("")) {}
    pcstr xr_stdcall getter() { return m_property; }
    void xr_stdcall setter(pcstr value)
    {
        xr_free(m_property);
        m_property = xr_strdup(value);
    }
};

static test_property2 s_test_property2;

pcstr s_properties3[] = {"one", "two", "three"};

static test_property2 s_test_property3;

struct test_property4
{
    bool m_property;

    test_property4() : m_property(false) {}
    bool xr_stdcall getter() { return m_property; }
    void xr_stdcall setter(bool value) { m_property = value; }
};

static test_property4 s_test_property4;

pcstr s_properties5[] = {"bad", "good"};

static test_property4 s_test_property5;

struct test_property6
{
    editor::color m_property;

    test_property6()
    {
        m_property.r = 0.f;
        m_property.g = 0.f;
        m_property.b = 1.f;
    }
    editor::color xr_stdcall getter() { return m_property; }
    void xr_stdcall setter(editor::color value) { m_property = value; }
};

static test_property6 s_test_property6;

struct test_property7
{
    float m_property;

    test_property7() : m_property(6.f) {}
    float xr_stdcall getter() { return m_property; }
    void xr_stdcall setter(float value) { m_property = value; }
};

static test_property7 s_test_property7;

static test_property7 s_test_property7_limited;

static test_property7 s_test_property7_values_enum;

std::pair<float, pcstr> s_properties7_enum[] = {std::make_pair(10.1f, "float_property_0"),
    std::make_pair(20.1f, "float_property_1"), std::make_pair(30.1f, "float_property_2")};

using editor::environment::manager;

manager::manager()
{
    // testing properties
    if (!Device.editor())
        return;

    editor::ide& ide = *Device.editor();
    editor::property_holder_base* holder = ide.create_property_holder();
    ide.active(holder);

    {
        holder->add_property("holder", "category", "description", holder);
    }

    {
        editor::property_holder_base::integer_getter_type getter;
        editor::property_holder_base::integer_setter_type setter;

        getter.bind(&s_test_property, &test_property::getter);
        setter.bind(&s_test_property, &test_property::setter);
        holder->add_property("integer", "category", "description", s_test_property.m_property, getter, setter);
    }

    {
        editor::property_holder_base::integer_getter_type getter;
        editor::property_holder_base::integer_setter_type setter;

        getter.bind(&s_test_property_limited, &test_property::getter);
        setter.bind(&s_test_property_limited, &test_property::setter);
        holder->add_property(
            "integer_limited", "category", "description", s_test_property_limited.m_property, getter, setter, 0, 10);
    }

    {
        editor::property_holder_base::integer_getter_type getter;
        editor::property_holder_base::integer_setter_type setter;

        getter.bind(&s_test_property_values, &test_property::getter);
        setter.bind(&s_test_property_values, &test_property::setter);
        holder->add_property("integer_values", "category", "description", s_test_property_limited.m_property, getter,
            setter, s_properties, 3);
    }

    {
        editor::property_holder_base::integer_getter_type getter;
        editor::property_holder_base::integer_setter_type setter;

        getter.bind(&s_test_property_enum, &test_property::getter);
        setter.bind(&s_test_property_enum, &test_property::setter);
        holder->add_property("integer_enum", "category", "description", s_test_property_enum.m_property, getter, setter,
            s_properties_enum, 3);
    }

    {
        editor::property_holder_base::string_getter_type getter;
        editor::property_holder_base::string_setter_type setter;

        getter.bind(&s_test_property2, &test_property2::getter);
        setter.bind(&s_test_property2, &test_property2::setter);
        holder->add_property("string", "category", "description", s_test_property2.m_property, getter, setter, ".dds",
            "Texture files (*.dds)|*.dds", "R:" DELIMITER "development" DELIMITER "priquel" DELIMITER "resources" DELIMITER "gamedata" DELIMITER "textures" DELIMITER "sky",
            "Select texture...");
    }

    {
        editor::property_holder_base::string_getter_type getter;
        editor::property_holder_base::string_setter_type setter;

        getter.bind(&s_test_property3, &test_property2::getter);
        setter.bind(&s_test_property3, &test_property2::setter);
        holder->add_property(
            "string_values", "category", "description", s_test_property3.m_property, getter, setter, s_properties3, 3);
    }

    {
        editor::property_holder_base::boolean_getter_type getter;
        editor::property_holder_base::boolean_setter_type setter;

        getter.bind(&s_test_property4, &test_property4::getter);
        setter.bind(&s_test_property4, &test_property4::setter);
        holder->add_property("boolean", "category", "description", s_test_property4.m_property, getter, setter);
    }

    {
        editor::property_holder_base::boolean_getter_type getter;
        editor::property_holder_base::boolean_setter_type setter;

        getter.bind(&s_test_property5, &test_property4::getter);
        setter.bind(&s_test_property5, &test_property4::setter);
        holder->add_property(
            "boolean_values", "category", "description", s_test_property5.m_property, getter, setter, s_properties5);
    }

    {
        editor::property_holder_base::color_getter_type getter;
        editor::property_holder_base::color_setter_type setter;

        getter.bind(&s_test_property6, &test_property6::getter);
        setter.bind(&s_test_property6, &test_property6::setter);
        holder->add_property("color", "category", "description", s_test_property6.m_property, getter, setter);
    }

    {
        editor::property_holder_base::float_getter_type getter;
        editor::property_holder_base::float_setter_type setter;

        getter.bind(&s_test_property7_limited, &test_property7::getter);
        setter.bind(&s_test_property7_limited, &test_property7::setter);
        holder->add_property("float", "category", "description", s_test_property7.m_property, getter, setter);
    }

    {
        editor::property_holder_base::float_getter_type getter;
        editor::property_holder_base::float_setter_type setter;

        getter.bind(&s_test_property7, &test_property7::getter);
        setter.bind(&s_test_property7, &test_property7::setter);
        holder->add_property(
            "float_limited", "category", "description", s_test_property7.m_property, getter, setter, 0.f, 1.f);
    }

    {
        editor::property_holder_base::float_getter_type getter;
        editor::property_holder_base::float_setter_type setter;

        getter.bind(&s_test_property7_values_enum, &test_property7::getter);
        setter.bind(&s_test_property7_values_enum, &test_property7::setter);
        holder->add_property("float_enum", "category", "description", s_test_property7_values_enum.m_property, getter,
            setter, s_properties7_enum, 3);
    }
}
