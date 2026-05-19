#include "dhwstate.h"

#include <cstring>

#include "constants.h"
#include "config.h"
#include "util.h"
#include "mqtttask.h"

namespace aquamqtt
{

/*** ENTITY ***/

Entity::Entity(DhwState* _device, const char* _entity_id, const char* _name, bool _is_diagnostic)
    : device(_device)
    , entity_id(_entity_id)
    , name(_name)
    , is_diagnostic(_is_diagnostic)
{
    snprintf(unique_id, MAX_UNIQUE_ID_SIZE, "%s_%s", _device->getDeviceId(), _entity_id);
    snprintf(state_topic, MAX_STATE_TOPIC_SIZE, "homeassistant/%s/state", unique_id);
    
    _def.insert({"unique_id", unique_id});
    _def.insert({"name", name});
    _def.insert({"state_topic", state_topic});

    if (is_diagnostic) {
        _def.insert({"entity_category", "diagnostic"});
    }
}

const char* Entity::state()
{
    if (_has_value) {
        return _state.c_str();
    } else {
        return nullptr;
    }
}

void Entity::unset_value()
{
    _has_value = false;
}

void Entity::update_state(const char* new_state)
{
    if (!_has_value && new_state == nullptr) {
        return; // no change
    }

    if (_has_value && new_state != nullptr && !strcmp(_state.c_str(), new_state)) {
        return; // no change
    }

    // state changed -> update values and notify MQTT
    _has_value = (new_state != nullptr);
    if (new_state) {
        _has_value = true;
        _state = new_state;
    } else {
        _has_value = false;
    }

    MqttTask::getInstance().queueUpdateEntity(this);
}

/*** BINARYSENSOR ***/

BinarySensor::BinarySensor(DhwState* _device, const char* _entity_id, const char* _name, bool _is_diagnostic)
    : Entity(_device, _entity_id, _name, _is_diagnostic)
    , value(false)
{
    _def.insert({"p", "binary_sensor"});
}

void BinarySensor::set_value(const bool new_value)
{
    value = new_value;
    update_state(value ? "on" : "off");
}

/*** SENSOR ***/

Sensor::Sensor(DhwState* _device, const char* _entity_id, const char* _name, bool _is_diagnostic)
    : Entity(_device, _entity_id, _name, _is_diagnostic)
{
    _def.insert({"p", "sensor"});
    _def.insert({"device_class", "temperature"});
    _def.insert({"unit_of_measurement", "°C"});
}

void Sensor::set_value(const float new_value)
{
    value = new_value;

    size_t n = snprintf(nullptr, 0, format, value) + 1;
    char buf[n];
    snprintf(buf, n, format, value);

    update_state(buf);
}

void Sensor::setFormat(const char* fmt)
{
    format = fmt;
}

/*** FILTEREDSENSOR ***/

FilteredSensor::FilteredSensor(DhwState* _device, const char* _entity_id, const char* _name, bool _is_diagnostic)
    : Sensor(_device, _entity_id, _name, _is_diagnostic)
    , filter(config::KALMAN_MEA_E, config::KALMAN_EST_E, config::KALMAN_Q)
{
    _def.insert({"p", "sensor"});
    _def.insert({"device_class", "temperature"});
    _def.insert({"unit_of_measurement", "°C"});

    setFormat("%.1f");
}

void FilteredSensor::set_value(const float new_value)
{
    float filtered = filter.updateEstimate(new_value);
    Sensor::set_value(filtered);
}

/*** DHWSTATE ***/

DhwState& DhwState::getInstance() {
    static DhwState instance(DEVICE_ID, DEVICE_NAME);
    return instance;
}

DhwState::DhwState(const char* _device_id, const char* _device_name)
    : device_id(_device_id)
    , device_name(_device_name)
{
    entities = {
        water_temperature = new FilteredSensor(this, "water_temperature", "Water Temperature", false),
        compressor_outlet_temperature = new FilteredSensor(this, "compressor_outlet_temperature", "Compressor Outlet Temperature", true),
        air_inlet_temperature = new FilteredSensor(this, "air_inlet_temperature", "Air inlet Temperature", true),
        evaporator1_temperature = new FilteredSensor(this, "evaporator1_temperature", "Evaporator1 Temperature", true),
        evaporator2_temperature = new FilteredSensor(this, "evaporator2_temperature", "Evaporator2 Temperature", true),
        evaporator3_temperature = new FilteredSensor(this, "evaporator3_temperature", "Evaporator3 Temperature", true),
        input_i2 = new BinarySensor(this, "input_i2", "Input I2", true),
        input_i1 = new BinarySensor(this, "input_i1", "Input I1", true),
        heating_active = new BinarySensor(this, "heating_active", "Heating Active", false)
    };
}


}  // namespace aquamqtt
