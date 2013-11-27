#include "Recognizer.h"

#include <tesseract/baseapi.h>

#include <QDebug>
#include <QSettings>

#include "Settings.h"

Recognizer::Recognizer(QObject *parent) :
  QObject(parent),
  engine_ (NULL), imageScale_ (0)
{
  applySettings ();
}

void Recognizer::applySettings()
{
  QSettings settings;
  settings.beginGroup (settings_names::recogntionGroup);

  tessDataDir_ = settings.value (settings_names::tessDataPlace,
                                 settings_values::tessDataPlace).toString ();
  if (tessDataDir_.right (1) != "/")
  {
    tessDataDir_ += "/";
  }
  ocrLanguage_ = settings.value (settings_names::ocrLanguage,
                                 settings_values::ocrLanguage).toString ();
  imageScale_ = settings.value (settings_names::imageScale,
                                settings_values::imageScale).toInt ();

  initEngine ();
}

bool Recognizer::initEngine()
{
  if (tessDataDir_.isEmpty () || ocrLanguage_.isEmpty ())
  {
    emit error (tr ("Неверные параметры для OCR"));
    return false;
  }
  if (engine_ != NULL)
  {
    delete engine_;
  }
  engine_ = new tesseract::TessBaseAPI();
  int result = engine_->Init(qPrintable (tessDataDir_), qPrintable (ocrLanguage_),
                             tesseract::OEM_DEFAULT);
  if (result != 0)
  {
    emit error (tr ("Ошибка инициализации OCR: %1").arg (result));
    delete engine_;
    engine_ = NULL;
    return false;
  }
  return true;
}

void Recognizer::recognize(ProcessingItem item)
{
  Q_ASSERT (!item.source.isNull ());
  if (engine_ == NULL)
  {
    if (!initEngine ())
    {
      return;
    }
  }

  QPixmap scaled = item.source;
  if (imageScale_ > 0)
  {
    scaled = scaled.scaledToHeight (scaled.height () * imageScale_,
                                    Qt::SmoothTransformation);
  }
  QImage image = scaled.toImage ();
  const int bytesPerPixel = image.depth () / 8;
  engine_->SetImage (image.bits (), image.width (), image.height (),
                     bytesPerPixel, image.bytesPerLine ());

  char* outText = engine_->GetUTF8Text();
  engine_->Clear();

  QString result (outText);
  result = result.trimmed();
  if (!result.isEmpty ())
  {
    item.recognized = result;
    emit recognized (item);
  }
  else
  {
    emit error (tr ("Текст не распознан."));
  }
  delete [] outText;
}