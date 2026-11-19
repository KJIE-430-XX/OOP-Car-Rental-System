FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY *.cpp *.h ./
COPY frontend ./frontend
COPY category_image ./category_image
COPY car_rental.db ./car_rental.db

RUN cmake -S . -B build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target CarRentalSystem -j"$(nproc)"

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends curl ca-certificates libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /src/build/CarRentalSystem /app/CarRentalSystem
COPY --from=build /src/frontend /app/frontend
COPY --from=build /src/category_image /app/category_image
COPY --from=build /src/car_rental.db /app/data/car_rental.db

ENV DB_PATH=/app/data/car_rental.db

EXPOSE 18080

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl --fail --silent --show-error http://127.0.0.1:18080/login.html >/dev/null || exit 1

CMD ["/app/CarRentalSystem"]
