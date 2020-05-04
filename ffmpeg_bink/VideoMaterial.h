
#include <QtQuick/qsgmaterialshader.h>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGMaterial>

#include "VideoFrame.h"

#ifndef GL_RED
#define GL_RED 0x1903
#endif

class QSGVideoMaterial;
class QSGVideoNode : public QSGGeometryNode {
 public:
  QSGVideoNode();
  ~QSGVideoNode();

  void setCurrentFrame(const SharedVideoFrame &frame);

  void setTexturedRectGeometry(const QRectF &boundingRect, const QRectF &textureRect);

 private:
  void bindTexture(int id, int unit, int w, int h, const uchar *bits);

  QRectF m_rect;
  QRectF m_textureRect;
  QSGVideoMaterial *m_material;
};

class QSGVideoMaterialShader : public QSGMaterialShader {
 public:
  const char *vertexShader() const override {
    return "uniform highp mat4 qt_Matrix;\n"
           "uniform highp float plane1Width;\n"
           "uniform highp float plane2Width;\n"
           "uniform highp float plane3Width;\n"
           "attribute highp vec4 qt_VertexPosition;\n"
           "attribute highp vec2 qt_VertexTexCoord;\n"
           "varying highp vec2 plane1TexCoord;\n"
           "varying highp vec2 plane2TexCoord;\n"
           "varying highp vec2 plane3TexCoord;\n"
           "\n"
           "void main() {\n"
           "    plane1TexCoord = qt_VertexTexCoord * vec2(plane1Width, 1);\n"
           "    plane2TexCoord = qt_VertexTexCoord * vec2(plane2Width, 1);\n"
           "    plane3TexCoord = qt_VertexTexCoord * vec2(plane3Width, 1);\n"
           "    gl_Position = qt_Matrix * qt_VertexPosition;\n"
           "}";
  }
  const char *fragmentShader() const override {
    return "uniform sampler2D plane1Texture;\n"
           "uniform sampler2D plane2Texture;\n"
           "uniform sampler2D plane3Texture;\n"
           "uniform mediump mat4 colorMatrix;\n"
           "uniform lowp float opacity;\n"
           "\n"
           "varying highp vec2 plane1TexCoord;\n"
           "varying highp vec2 plane2TexCoord;\n"
           "varying highp vec2 plane3TexCoord;\n"
           "\n"
           "void main()\n"
           "{\n"
           "    mediump float Y = texture2D(plane1Texture, plane1TexCoord).r;\n"
           "    mediump float U = texture2D(plane2Texture, plane2TexCoord).r;\n"
           "    mediump float V = texture2D(plane3Texture, plane3TexCoord).r;\n"
           "    mediump vec4 color = vec4(Y, U, V, 1.);\n"
           "    gl_FragColor = colorMatrix * color * opacity;\n"
           "}";
  }

  char const *const *attributeNames() const override {
    static const char *names[] = {"qt_VertexPosition", "qt_VertexTexCoord", 0};
    return names;
  }

  void updateState(const RenderState &state, QSGMaterial *newMaterial,
                   QSGMaterial *oldMaterial) override;

 protected:
  void initialize() override {
    m_id_matrix = program()->uniformLocation("qt_Matrix");
    m_id_plane1Width = program()->uniformLocation("plane1Width");
    m_id_plane2Width = program()->uniformLocation("plane2Width");
    m_id_plane3Width = program()->uniformLocation("plane3Width");
    m_id_plane1Texture = program()->uniformLocation("plane1Texture");
    m_id_plane2Texture = program()->uniformLocation("plane2Texture");
    m_id_plane3Texture = program()->uniformLocation("plane3Texture");
    m_id_colorMatrix = program()->uniformLocation("colorMatrix");
    m_id_opacity = program()->uniformLocation("opacity");
  }

  int m_id_matrix;

  int m_id_plane1Width;
  int m_id_plane2Width;
  int m_id_plane3Width;

  int m_id_plane1Texture;
  int m_id_plane2Texture;
  int m_id_plane3Texture;

  int m_id_colorMatrix;
  int m_id_opacity;
};

class QSGVideoMaterial : public QSGMaterial {
 public:
  QSGVideoMaterial();
  ~QSGVideoMaterial();

  QSGMaterialType *type() const override {
    static QSGMaterialType triPlanarType;
    return &triPlanarType;
  }

  QSGMaterialShader *createShader() const override {
    return new QSGVideoMaterialShader;
  }

  int compare(const QSGMaterial *other) const override {
    const auto *m = static_cast<const QSGVideoMaterial *>(other);
    if (!m_textureIds[0])
      return 1;

    int d = m_textureIds[0] - m->m_textureIds[0];
    if (d)
      return d;
    else if ((d = m_textureIds[1] - m->m_textureIds[1]) != 0)
      return d;
    else
      return m_textureIds[2] - m->m_textureIds[2];
  }

  void updateBlending() { setFlag(Blending, qFuzzyCompare(m_opacity, qreal(1.0)) ? false : true); }

  void setCurrentFrame(const SharedVideoFrame &frame) {
    QMutexLocker lock(&m_frameMutex);
    m_frame = frame;
  }

  void bind();
  void bindTexture(int id, int w, int h, const uchar *bits, GLenum format);

  QSize m_textureSize;
  int m_planeCount;

  GLuint m_textureIds[3];
  GLfloat m_planeWidth[3];

  qreal m_opacity;
  QMatrix4x4 m_colorMatrix;

  SharedVideoFrame m_frame;
  QMutex m_frameMutex;
};
