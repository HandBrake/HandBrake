CURDIR      ?= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
TOPDIR      ?= $(CURDIR)/../..
RT          ?= win-x64
CONF        ?= Release
PROPFILE    ?= Directory.Build.props
SRC_DIRS    ?= HandBrake.App.Core HandBrake.Interop HandBrake.Worker HandBrakeWPF

build: clean $(PROPFILE) force
	cd $(CURDIR) && dotnet build -c $(CONF) --self-contained true
	cd $(CURDIR) && dotnet publish $(CURDIR)/$(lastword $(SRC_DIRS)) -c $(CONF) --self-contained true -p:PublishSingleFile=true
	$(if $(wildcard $(TOPDIR)/build/libhb/*.dll $(TOPDIR)/build/contrib/bin/*.dll),cp $(wildcard $(TOPDIR)/build/libhb/*.dll $(TOPDIR)/build/contrib/bin/*.dll) $(CURDIR)/$(lastword $(SRC_DIRS))/bin/$(CONF)/$(RT)/publish)

clean: restore force
	cd $(CURDIR) && dotnet clean

restore: force
	cd $(CURDIR) && dotnet restore

$(PROPFILE): force
	@printf '<Project>\n' > $(CURDIR)/$(PROPFILE)
	@printf '  <PropertyGroup>\n' >> $(CURDIR)/$(PROPFILE)
	@printf '    <RuntimeIdentifier>$(RT)</RuntimeIdentifier>\n' >> $(CURDIR)/$(PROPFILE)
	@printf '  </PropertyGroup>\n' >> $(CURDIR)/$(PROPFILE)
	@printf '</Project>\n' >> $(CURDIR)/$(PROPFILE)

xclean: force
	$(foreach dir,$(SRC_DIRS), \
		rm -rf $(CURDIR)/$(dir)/bin/* $(CURDIR)/$(dir)/obj/*; \
	)

.PHONY: all build clean xclean force restore
force:
