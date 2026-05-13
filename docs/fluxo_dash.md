# Fluxo DASH implementado

```text
Cliente DASH                         Servidor HTTP/DASH
     |                                      |
     | GET /stream.mpd                     |
     |------------------------------------->|
     | 200 OK + MPD                        |
     |<-------------------------------------|
     |                                      |
     | Parser do MPD                        |
     | Lista representações                 |
     |                                      |
     | GET /seg_1_1.m4s                     |
     |------------------------------------->|
     | 200 OK + segmento 360p              |
     |<-------------------------------------|
     | Mede throughput                      |
     |                                      |
     | GET /seg_2_2.m4s ou /seg_3_2.m4s     |
     |------------------------------------->|
     | 200 OK + segmento escolhido          |
     |<-------------------------------------|
     | Atualiza histórico ABR               |
```

O cliente não baixa um único arquivo de vídeo inteiro. Ele baixa pequenos segmentos. Essa divisão permite trocar a qualidade durante a reprodução conforme a banda disponível.
