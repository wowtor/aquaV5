#ifndef AQUAMQTT_DHWSTATE_H
#define AQUAMQTT_DHWSTATE_H

#include <string>
#include <map>
#include <vector>

#include <SimpleKalmanFilter.h>


#define MAX_UNIQUE_ID_SIZE 60
#define MAX_STATE_TOPIC_SIZE (MAX_UNIQUE_ID_SIZE+30)


namespace aquamqtt
{

class DhwState;

class Entity
{
private:
    DhwState* device;
    const char* entity_id;
    const char* name;
    bool is_diagnostic;

    char unique_id[MAX_UNIQUE_ID_SIZE];
    char state_topic[MAX_STATE_TOPIC_SIZE];

    bool _has_value = false;
    std::string _state;

public:
    Entity(DhwState* device, const char* entity_id, const char* name, bool is_diagnostic);
    virtual ~Entity() = default;

    const char* state();
    void unset_value();

    inline const DhwState& getDevice() { return *device; };
    inline const char* getUniqueId() const { return unique_id; };
    inline const char* getStateTopic() const { return state_topic; };

    inline const std::map<std::string, std::string> &definition() const { return _def; };

protected:
    std::map<std::string, std::string> _def;
    void update_state(const char* new_state);
};


class BinarySensor: public Entity
{
private:
    bool value;

public:
    BinarySensor(DhwState* device_id, const char* entity_id, const char* name, bool is_diagnostic);
    virtual ~BinarySensor() = default;

    void set_value(const bool new_value);
};


class Sensor: public Entity
{
private:
    float value = 0;
    const char* format = "%f";
public:

    Sensor(DhwState* device_id, const char* entity_id, const char* name, bool is_diagnostic);
    virtual ~Sensor() = default;

    virtual void set_value(const float new_value);
    void setFormat(const char* fmt);
};

class FilteredSensor: public Sensor
{
private:
    SimpleKalmanFilter filter; // See: https://github.com/denyssene/SimpleKalmanFilter
public:

    FilteredSensor(DhwState* device_id, const char* entity_id, const char* name, bool is_diagnostic);
    virtual ~FilteredSensor() = default;

    virtual void set_value(const float new_value);
};


class DhwState final
{
private:
    std::string device_id;
    std::string device_name;

public:
    static DhwState& getInstance();

    inline const char* getDeviceId() const { return device_id.c_str(); };
    inline const char* getDeviceName() const { return device_name.c_str(); };

    Sensor* water_temperature;
    Sensor* compressor_outlet_temperature;
    Sensor* air_inlet_temperature;
    Sensor* evaporator1_temperature;
    Sensor* evaporator2_temperature;
    Sensor* evaporator3_temperature;

    BinarySensor* input_i2;
    BinarySensor* input_i1;
    BinarySensor* heating_active;

    std::vector<Entity*> entities;

private:
    DhwState(const char* device_id, const char* device_name);
    ~DhwState() = default;
};

}  // namespace aquamqtt

#endif  // AQUAMQTT_DHWSTATE_H
