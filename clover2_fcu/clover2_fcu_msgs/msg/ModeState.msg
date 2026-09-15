# Currently active mode, published by the clover2_fcu bridge (transient
# local, late-joining subscribers get the current state immediately).

# 0 = no active mode
uint32 mode_id

# Registered mode name, empty when no mode is active
string name
