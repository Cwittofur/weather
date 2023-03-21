# Build stage
FROM golang:1.16-alpine AS builder
WORKDIR /app
COPY main.go .
RUN go build -o myapp

# Run stage
FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /app
COPY --from=builder /app/myapp .
CMD ["/app/myapp"]