MODULES = ass2
EXTENSION = ass2
DATA = ass2--0.0.1.sql
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)