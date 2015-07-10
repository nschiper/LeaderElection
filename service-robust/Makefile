all:	omega libomega

omega:
	@if test -d src; then\
		cd src; \
		echo "Compiling the omega service";\
		make service-robust; \
		cd ..;\
	fi

libomega:
	@if test -d omegalib; then \
		cd omegalib; \
		echo "Compiling the omega shared library" ;\
		make ; \
		cd ..; \
	fi;

install_lib:
	@if test -d /usr/local/lib; then \
		if test -f omegalib/libservice-robust.so; then\
			cp omegalib/libservice-robust.so /usr/local/lib;\
		else\
			echo "Installation failed" ;\
			exit 1 ;\
		fi;\
	else \
		echo "Installation failed" ;\
		exit 1;\
	fi;

install: install_lib
	@if test -d /usr/local/bin; then \
		if test -f src/service-robust; then \
			cp src/service-robust /usr/local/bin; \
			cp include/service-robustlib.h /usr/local/include; \
		else \
			echo "Installation failed" ;\
			exit 1 ;\
		fi;\
	else \
		echo "Installation failed"; \
		exit 1 ; \
	fi ;

uninstall:
	@if test -f /usr/local/bin/service-robust; then \
		$(RM) /usr/local/bin/service-robust; \
	fi;
	@if test -f /usr/local/lib/libservice-robust.so; then\
		$(RM) /usr/local/lib/libservice-robust.so; \
	fi;
	@if test -f /usr/local/include/service-robustlib.h; then\
		$(RM) /usr/local/include/service-robustlib.h; \
	fi;	

clean:
	@if test -d src; then \
		cd src; \
		make clean; \
		cd ..; \
	fi;

	@if test -d omegalib; then \
		cd omegalib; \
		make clean ;\
		cd ..;\
	fi ;



