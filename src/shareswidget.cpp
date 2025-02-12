#include "shareswidget.h"

#include "intervalmodel.h"
#include "period.h"
#include "projectmodel.h"
#include "timesheet.h"
#include <QPainter>
#include <QPainterPath>
#include <ranges>

SharesWidget::SharesWidget(QWidget* parent) : QWidget(parent)
{
}

void SharesWidget::update(const TimeSheet& time_sheet, const Period& period)
{
  m_shares.clear();
  const auto& interval_model = time_sheet.interval_model();
  double total = 0.0;
  for (const auto& project : time_sheet.project_model().projects()) {
    using std::chrono_literals::operator""min;
    const auto d = interval_model.minutes(period, project->name()) / 1.min;
    m_shares.emplace_back(project, d);
    total += d;
  }
  for (auto& value : m_shares | std::views::values) {
    value /= total;
  }
  update();
}

void SharesWidget::paintEvent(QPaintEvent* event)
{
  const auto s = size();
  const auto d = static_cast<double>(std::min(s.width(), s.height()));
  const auto left = (s.width() - d) / 2.0;
  const auto top = (s.height() - d) / 2.0;
  const auto rect = QRectF{QPointF{left, top}, QSizeF{d, d}};
  const auto pos_on_circle = [center = rect.center(), r = d / 2.0](const auto degrees) {
    const auto rad = degrees * M_PI / 180.0;
    return r * QPointF(std::cos(rad), -std::sin(rad)) + center;
  };

  QPainter painter{this};
  painter.setRenderHint(QPainter::Antialiasing);
  auto start_angle = 0.0;
  QPainterPath outline;
  for (const auto& [project, share] : m_shares) {
    QPainterPath piece_of_cake;
    const auto end_angle = start_angle + share * 360.0;
    piece_of_cake.moveTo(rect.center());
    piece_of_cake.arcTo(rect, start_angle, end_angle - start_angle);
    piece_of_cake.lineTo(rect.center());
    outline.moveTo(rect.center());
    outline.lineTo(pos_on_circle(start_angle));
    painter.fillPath(piece_of_cake, project->color());
    start_angle = end_angle;
  }
  outline.addEllipse(rect);

  QPen pen;
  pen.setCapStyle(Qt::RoundCap);
  pen.setColor(Qt::black);
  pen.setWidth(3);
  painter.setPen(pen);
  painter.drawPath(outline);
}
