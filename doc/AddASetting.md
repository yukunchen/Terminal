# Adding a Settings Property

1. Add to winconp.h
    * define registry name (ex `CONSOLE_REGISTRY_CURSORCOLOR`)
    * add the setting to `CONSOLE_STATE_INFO`
    * define the property key ID and the property key itself
        - Yes, the large majority of the `DEFINE_PROPERTYKEY` defs are the same, it's only the last byte of the guid that changes
2. Add matching fields to Settings.hpp
    - add getters, setters, the whole dirll.
3. Add to the propsheet registry handler, `src/propsheet/registry.cpp`
4. Add the field to the propslib registry map
5. Add the value to `ShortcutSerialization.cpp`
    - Read the value in `ShortcutSerialization::s_PopulateV2Properties`
    - Write the value in `ShortcutSerialization::s_SetLinkValues`

Now, your new setting should be stored just like all the other properties.
