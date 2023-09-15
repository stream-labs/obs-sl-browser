#pragma once

#include <QWidget>

class SlBrowserWidget : public QWidget
{
public:
	SlBrowserWidget();

protected:
	void closeEvent(QCloseEvent *event) override;
};
