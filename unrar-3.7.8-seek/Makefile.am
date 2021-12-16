
SUBDIRS=win32

bin_PROGRAMS=unrar

unrardir=$(prefix)/etc

unrar_SOURCES= archive.cpp archive.hpp arcread.cpp array.hpp			\
 cmddata.cpp cmddata.hpp compress.hpp consio.cpp consio.hpp crc.cpp		\
 crc.hpp crypt.cpp crypt.hpp encname.cpp encname.hpp errhnd.cpp			\
 errhnd.hpp extinfo.cpp extinfo.hpp extract.cpp extract.hpp				\
 filcreat.cpp filcreat.hpp file.cpp file.hpp filefn.cpp filefn.hpp		\
 filestr.cpp filestr.hpp find.cpp find.hpp getbits.cpp getbits.hpp		\
 global.cpp global.hpp headers.hpp int64.cpp int64.hpp isnt.cpp			\
 isnt.hpp list.cpp list.hpp loclang.hpp match.cpp match.hpp				\
 options.cpp options.hpp os.hpp pathfn.cpp pathfn.hpp rar.cpp rar.hpp	\
 rardefs.hpp rarfn.hpp rarlang.hpp raros.hpp rartypes.hpp rarvm.cpp		\
 rarvm.hpp rawread.cpp rawread.hpp rdwrfn.cpp rdwrfn.hpp recvol.cpp		\
 recvol.hpp resource.cpp resource.hpp rijndael.cpp rijndael.hpp rs.cpp	\
 rs.hpp savepos.cpp savepos.hpp scantree.cpp scantree.hpp sha1.cpp		\
 sha1.hpp strfn.cpp strfn.hpp strlist.cpp strlist.hpp system.cpp		\
 system.hpp timefn.cpp timefn.hpp ulinks.cpp ulinks.hpp unicode.cpp		\
 unicode.hpp unpack.cpp unpack.hpp version.hpp volume.cpp volume.hpp


EXTRA_DIST=license.txt readme.txt arccmt.cpp beosea.cpp coder.cpp		\
      coder.hpp dll.cpp dll.hpp model.cpp model.hpp os2ea.cpp			\
      rarvmtbl.cpp suballoc.cpp suballoc.hpp unios2.cpp unpack15.cpp	\
      uowners.cpp win32acl.cpp win32stm.cpp log.cpp log.hpp				\
      smallfn.cpp smallfn.hpp unpack20.cpp

# We also install 'unrar' in the main llink, for nicety.
install-data-hook:
	@$(INSTALL_PROGRAM) -C "$(builddir)/unrar" "$(unrardir)/llink"
