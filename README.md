# rate_cms

This project implements a rate limiter using the Count-Min Sketch algorithm in C. It is a reimplementation of Cloudflare's rate-limiting approach from the Pingora project (written in Rust, [pingora-limits](https://github.com/cloudflare/pingora/tree/main/pingora-limits)).

The Count-Min Sketch implementation used here is based on the count-min-sketch [repository](https://github.com/barrust/count-min-sketch) by barrust.

## Usage

The `rate_test.c` show how the function work.

```bash
cd src
make
rate_test
```

## License

This project is licensed under the MIT License. See LICENSE for more details.
