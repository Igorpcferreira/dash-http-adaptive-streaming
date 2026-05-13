# Exercício 15 - HTTP, TCP e DASH

O HTTP tradicional não foi criado especificamente para streaming contínuo de mídia. Como ele utiliza TCP, sofre os efeitos do controle de congestionamento do protocolo, que pode gerar variação na taxa de transmissão. Esse comportamento, frequentemente descrito como serrilhado, ocorre porque a janela de congestionamento cresce enquanto a rede parece estável e diminui quando há perda, congestionamento ou erro de canal.

Em streaming multimídia, essa oscilação pode causar travamentos. Se o cliente tentar consumir mídia em uma taxa maior do que a taxa efetivamente entregue pela rede, o buffer esvazia e a reprodução para.

O DASH reduz esse problema dividindo o conteúdo em segmentos pequenos e disponibilizando múltiplas representações do mesmo vídeo, cada uma com bitrate e resolução diferentes. O cliente mede a condição da rede durante o download dos segmentos e escolhe dinamicamente a melhor qualidade possível. Se a banda cair, baixa uma representação menor. Se a banda melhorar, pode subir a qualidade.

Além disso, o DASH adiciona recursos ausentes no HTTP básico:

- manifesto MPD descrevendo duração, segmentos e representações disponíveis;
- segmentação temporal do conteúdo;
- seleção adaptativa de bitrate no cliente;
- suporte a troca dinâmica de qualidade;
- uso de infraestrutura HTTP comum, como proxies, caches e CDNs;
- maior tolerância a congestionamento e variação de rede.

Assim, o DASH mantém a simplicidade de distribuição via HTTP, mas adiciona a lógica necessária para streaming adaptativo de mídia.
