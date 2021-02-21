\echo Use "ALTER EXTENSION ulid UPDATE TO '1.1'" to load this file. \quit

ALTER FUNCTION  ulid_generate() VOLATILE;
ALTER FUNCTION  ulid_to_string(uuid) IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ulid_from_string(IN id text)
RETURNS uuid
AS 'libulid', 'ulid_from_string'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
