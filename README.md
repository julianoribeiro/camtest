# camtest

## Para converter o modelo de TensorFlow para TFLite:

```
tflite_convert \
  --saved_model_dir=saved_model_directory \
  --output_file=model.tflite
  ```

* **saved_model_directory**: É o diretório onde o modelo TensorFlow salvo está armazenado.
* **model.tflite**: É o nome do arquivo do modelo TFLite de saída.

## Para migrar o modelo de tflite para C:
```
xxd -i model.tflite > model.h
```
