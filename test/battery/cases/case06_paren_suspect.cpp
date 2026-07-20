TEST_MACRO(foo)
static void Helper(){
	run();
}
// EXPECT-SUSPECT 2:4.3
