import tensorflow as tf
from tensorflow.keras import layers, models
from tensorflow.keras.preprocessing.image import ImageDataGenerator
import matplotlib.pyplot as plt
import numpy as np
import json

DATASET_DIR = 'dataset'
IMG_SIZE    = (96, 96)
BATCH_SIZE  = 16
MODEL_PATH  = 'model_fructe.keras'

print("Incarcare dataset...")


datagen_train = ImageDataGenerator(
    rescale=1./255,
    validation_split=0.2,
    rotation_range=15,
    width_shift_range=0.1,
    height_shift_range=0.1,
    horizontal_flip=True,
    zoom_range=0.15,
    brightness_range=[0.85, 1.15]
)

datagen_val = ImageDataGenerator(
    rescale=1./255,
    validation_split=0.2
)

train_data = datagen_train.flow_from_directory(
    DATASET_DIR,
    target_size=IMG_SIZE,
    batch_size=BATCH_SIZE,
    class_mode='categorical',
    subset='training',
    shuffle=True
)

val_data = datagen_val.flow_from_directory(
    DATASET_DIR,
    target_size=IMG_SIZE,
    batch_size=BATCH_SIZE,
    class_mode='categorical',
    subset='validation',
    shuffle=False
)

NUM_CLASSES = len(train_data.class_indices)
print(f"Clase: {train_data.class_indices}")
print(f"Imagini antrenare: {train_data.samples}")
print(f"Imagini validare:  {val_data.samples}")

# Model
base_model = tf.keras.applications.MobileNetV2(
    input_shape=(96, 96, 3),
    # nu vrem stratul final, noi avem 3 clase
    include_top=False,
    weights='imagenet'
)
base_model.trainable = False

inputs  = tf.keras.Input(shape=(96, 96, 3))
x = base_model(inputs, training=False)
x = layers.GlobalAveragePooling2D()(x)
x = layers.Dense(64, activation='relu')(x)
x = layers.Dropout(0.3)(x)
outputs = layers.Dense(NUM_CLASSES, activation='softmax')(x)

model = tf.keras.Model(inputs, outputs)

model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    loss='categorical_crossentropy',
    metrics=['accuracy']
)

# Antrenare
callbacks = [
    tf.keras.callbacks.EarlyStopping(
        monitor='val_accuracy',
        patience=5,
        restore_best_weights=True,
        verbose=1
    ),
    tf.keras.callbacks.ModelCheckpoint(
        MODEL_PATH,
        monitor='val_accuracy',
        save_best_only=True,
        verbose=1
    )
]

history = model.fit(
    train_data,
    epochs=20,
    validation_data=val_data,
    callbacks=callbacks,
    verbose=1
)

# Salvare
model.save(MODEL_PATH)
print(f"\nModel salvat: {MODEL_PATH}")

with open('class_indices.json', 'w') as f:
    json.dump(train_data.class_indices, f)
print("Clase salvate: class_indices.json")

# Rezultate
val_loss, val_acc = model.evaluate(val_data, verbose=0)
print(f"\nVal Accuracy: {val_acc*100:.1f}%")
print(f"Val Loss:     {val_loss:.4f}")

# Grafic
plt.figure(figsize=(12, 4))
plt.subplot(1, 2, 1)
plt.plot(history.history['accuracy'], label='Train')
plt.plot(history.history['val_accuracy'], label='Validation')
plt.title('Accuracy')
plt.legend()
plt.subplot(1, 2, 2)
plt.plot(history.history['loss'], label='Train')
plt.plot(history.history['val_loss'], label='Validation')
plt.title('Loss')
plt.legend()
plt.savefig('training_history.png')
plt.show()