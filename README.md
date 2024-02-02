# traffic-generator

## application modules
- the public interface of the given modules is provided by the components at the top level and the components of the lower levels are just an implementation detail
- the component levels are not specified in the below diagram to not make it even more "complex"
- the logging is used only by the top level components of a given module and not by the private/implementation modules
- the project utilities module is used by whatever components need given utility
- the generation operations interface (`gen::priv::generation_ops`) is used to break cyclic dependencies between the `mgmt::manager` and the lower level components of the `mgmt` module
- the `mgmt::priv::event_handle` is just a thin wrapper/facade around the `mgmt::priv::event_scheduler` which is used to limit the provided operations because it's not correct for the `gen::priv::flows_generator` to use the full capabilities of the event scheduler
- the source of the below diagram can be found [here](https://excalidraw.com/#json=3Jy7861Yp2jh4pcZ3Zsdu,roswOApg9K7IWUP1NuxLpQ)
![scheme](https://github.com/freakpv/traffic-generator/blob/main/modules.png)
