# Universally Unique Lexicographically Sortable Identifier generate for PostgreSQL

Extension for generate [ULIDs](https://github.com/ulid/spec).

## Usage

| Function                         | Return Type | Example                                | Result                               |
|:---------------------------------|:------------|:---------------------------------------|:-------------------------------------|
| ulid_generate()                  | uuid        | select ulid_generate()                 | 0177a26f-6db1-11bf-54e2-e0f29ca6fc72 |
| ulid_to_string(id uuid)          | text        | select ulid_to_string(ulid_generate()) | 01EYH71504XTJ2J812EEVM               |

## Build

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
    make
    make install

## Examples

```postgresql
CREATE EXTENSION ulid;
-- ...
CREATE TABLE foo
(
    id uuid default ulid_generate() primary key,
    value bigint not null
);
-- ...
INSERT INTO foo(value) SELECT generate_series(1, 1000) as value ;
-- ...
SELECT ulid_to_string(id), value FROM foo ORDER BY id LIMIT 10;
```