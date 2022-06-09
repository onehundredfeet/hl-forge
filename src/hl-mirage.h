

void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));

/*
static void gctrl_onDetect( vclosure *onDetect, dx_gctrl_device *device, const char *name) {
	if( !onDetect ) return;
	if( onDetect->hasValue )
		((void(*)(void *, dx_gctrl_device*, vbyte*))onDetect->fun)(onDetect->value, device, (vbyte*)name);
	else
		((void(*)(dx_gctrl_device*, vbyte*))onDetect->fun)(device, (vbyte*)name);
}
*/