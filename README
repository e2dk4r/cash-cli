# Build

You can look [Dockerfile](./Dockerfile) to build natively
If you use docker
```docker
docker build -t cash-cli .
```

# Run

```shell
docker run --rm -e API_KEY=<YOUR-KEY> cash-cli cash <amount> <from> <to>
```

# Scripts
You can add your powershell profile this function
```powershell
function cash {
    param([int]$Amount, [string]$From, [string]$To)
    docker run --rm -e API_KEY=<YOUR-KEY> cash-cli cash $Amount $From $To
}
```
