MAC_CC = clang
IOS_CC = xcrun --sdk iphoneos clang

IOS_CODESIGN = codesign
IOS_ENT = ent.plist

MAC_ARCH = -arch arm64 -arch x86_64
IOS_ARCH = -arch armv7 -arch arm64

CFLAGS = -O3
CFLAGS += -Ililirecovery
LDFLAGS = -framework IOKit -framework CoreFoundation

IOS_CFLAGS = -miphoneos-version-min=6.0
IOS_CFLAGS += -Iinclude
IOS_IOKIT_LINK = ln -fsh $(shell xcrun --sdk macosx --show-sdk-path)/System/Library/Frameworks/IOKit.framework/Versions/Current/Headers ./include/IOKit

MAC_CFLAGS = -mmacos-version-min=10.8

CHECKM8_SRC = checkm8.c lilirecovery/lilirecovery.c
CHECKM8_IOS_BUILD = build/obscurantistic_checkm8_ios
CHECKM8_MAC_BUILD = build/obscurantistic_checkm8

DIR_HELPER = mkdir -p $(@D)

.PHONY: all ios mac clean

all: ios mac
	@echo "done!"

ios: $(CHECKM8_IOS_BUILD)

mac: $(CHECKM8_MAC_BUILD)

$(CHECKM8_IOS_BUILD): $(CHECKM8_SRC)
	@echo "\tbuilding obscurantistic_checkm8 for iOS"
	@$(DIR_HELPER)
	@$(IOS_IOKIT_LINK)
	@$(IOS_CC) $(IOS_ARCH) $(IOS_CFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@$(IOS_CODESIGN) -s - -f --entitlements $(IOS_ENT) $@

$(CHECKM8_MAC_BUILD): $(CHECKM8_SRC)
	@echo "\tbuilding obscurantistic_checkm8 for Mac"
	@$(DIR_HELPER)
	@$(MAC_CC) $(MAC_ARCH) $(MAC_CFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	@rm -rf build
	@echo "done cleaning!"
