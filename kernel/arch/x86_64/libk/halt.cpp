void halt() {
    for (;;) {
        asm("hlt");
    }
}

