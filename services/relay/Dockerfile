# Build stage
FROM golang:1.19-alpine AS builder
WORKDIR /app
RUN echo $(ls -l .)
COPY main.go .
COPY go.mod .
RUN go mod tidy
RUN go build -o myapp

# Run stage
FROM alpine:latest AS runner
RUN apk --no-cache add ca-certificates
WORKDIR /app
COPY --from=builder /app/myapp .
CMD ["/app/myapp"]