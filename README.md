# system-call-http

## 起動コマンド

```bash
gcc ./calc_server.c -o ./calc_server && ./calc_server
```

## 動作確認コマンド例

```bash
curl "http://localhost:8000/calc?query=2+10"
```
