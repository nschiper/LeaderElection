all:	omega libomega

omega:
	@if test -d src; then\
		cd src; \
		echo "Compiling the omega service";\
		make service-scalable; \
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
		if test -f omegalib/libservice-scalable.so; then\
			cp omegalib/libservice-scalable.so /usr/local/lib;\
			cp include/service-scalablelib.h /usr/local/include; \
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
		if test -f src/service-scalable; then \
			cp src/service-scalable /usr/local/bin; \
		else \
			echo "Installation failed" ;\
			exit 1 ;\
		fi;\
	else \
		echo "Installation failed"; \
		exit 1 ; \
	fi ;

uninstall:
	@if test -f /usr/local/bin/service-scalable; then \
		$(RM) /usr/local/bin/service-scalable; \
	fi;
	@if test -f /usr/local/lib/libservice-scalable.so; then\
		$(RM) /usr/local/lib/libservice-scalable.so; \
	fi;
	@if test -f /usr/local/include/service-scalablelib.h; then\
		$(RM) /usr/local/include/service-scalablelib.h; \
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



