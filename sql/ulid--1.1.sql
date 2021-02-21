\echo Use "CREATE EXTENSION pg_ulid" to load this file. \quit

CREATE OR REPLACE FUNCTION ulid_generate()
RETURNS uuid
AS 'libulid', 'ulid_generate'
LANGUAGE C VOLATILE;

CREATE OR REPLACE FUNCTION ulid_to_string(IN id uuid)
RETURNS text
AS 'libulid', 'ulid_to_string'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ulid_from_string(IN id text)
RETURNS uuid
AS 'libulid', 'ulid_from_string'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
