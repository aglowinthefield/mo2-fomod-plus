#ifndef FOMODIMAGEVIEWER_H
#define FOMODIMAGEVIEWER_H

#include <QDialog>

using LabelImagePair = std::pair<QString&, QString&>;

class FomodImageViewer final : public QDialog
{
    Q_OBJECT
public:
    explicit FomodImageViewer(QWidget *parent);

private:

    std::vector<LabelImagePair> mLabelsAndImages;

};



#endif //FOMODIMAGEVIEWER_H
