CREATE FUNCTION ass2(            
    OUT vendor text,
    OUT model_name text,
    OUT physical_processor int,
    OUT no_of_cores int,
    OUT clock_speed_hz int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION ass2() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION ass2() TO postgres;