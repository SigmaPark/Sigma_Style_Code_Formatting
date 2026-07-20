TEST_MACRO(foo)

static void Helper(){
	run();
}
// EXPECT-SUSPECT 3:4.3
