##Carichiamo i pacchetti
library(ggplot2)

vibrazioni <- read.csv('C:\\Users\\Jody\\PycharmProjects\\Algoritmi\\prog ele\\logs\\dati_stm32.csv', head=T)
vibrazioni

vibrazioni <- vibrazioni[vibrazioni$ms > 10000, ]
vibrazioni
head(vibrazioni,10)


vibrazioni$modulo <- sqrt(vibrazioni$ax_mg^2 + vibrazioni$ay_mg^2 + vibrazioni$az_mg^2)

vibrazioni$g <- vibrazioni$modulo / 1000


vibrazioni$g_centered <- vibrazioni$g - mean(vibrazioni$g, na.rm = TRUE)


vibrazioni$tempo <- 0.5 * seq_len(nrow(vibrazioni))





n_allarmi <- sum(vibrazioni$status == "ALARM")
n_allarmi

modulo_tutte_medie <- mean(vibrazioni$modulo)
modulo_tutte_medie

righe_allarmi <- vibrazioni[vibrazioni$status == "ALARM", ]
righe_allarmi


messaggio <- paste(
  "Numero allarmi:", nrow(righe_allarmi),
  "al tempo",
  if (nrow(righe_allarmi) > 0) {
    paste(righe_allarmi$tempo, collapse = ", ")
  } else {
    "—"
  }
)




baseline <- mean(vibrazioni$modulo, na.rm = TRUE)

# 2) Creo la variabile centrata: valori sopra la media → positivi, sotto → negativi
vibrazioni$g_centered <- vibrazioni$modulo - baseline

ggplot(vibrazioni, aes(x = tempo, y = g_centered)) +
  geom_line() +
  geom_point(aes(color = status)) +
  scale_color_manual(values = c("ALARM" = "red", "OK" = "black")) +
  scale_x_continuous(breaks = seq(0,max(vibrazioni$tempo), 5)) +
  labs(x = "Secondi", y = "Deviazione dalla media [g]") +
  geom_hline(yintercept = 0, linetype = "dashed", color = "blue")






#ggplot(data = vibrazioni, mapping = aes(x = tempo, y = modulo )) + geom_line() +
# scale_x_continuous(breaks = seq(0, max(vibrazioni$tempo), 10)) +
#geom_point(aes(color = status)) +
#scale_color_manual(values = c("ALARM" = "red", "OK" = "black"))




#ggplot(data = vibrazioni, mapping = aes(x = tempo, y = g_centered )) + geom_line() +
# labs(x = "Secondi", y = "Deviazione dalla modulo [g]") +
#scale_x_continuous(breaks = seq(0, 120, 5)) +
#geom_point(aes(color = status)) +
#scale_color_manual(values = c("ALARM" = "red", "OK" = "black"))  
#+
#annotate("text", x = mean(vibrazioni$tempo)*0.9, y = max(vibrazioni$modulo),
#label = messaggio,
#color = "blue", size = 3, hjust = 0)






cat("\014")  # comando che pulisce la console

print(summary(vibrazioni))
print(messaggio)








