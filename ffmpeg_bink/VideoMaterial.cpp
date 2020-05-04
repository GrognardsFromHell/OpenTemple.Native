
#include "VideoMaterial.h"

QSGVideoMaterial::QSGVideoMaterial() : m_opacity(1.0) {
  memset(m_textureIds, 0, sizeof(m_textureIds));
  m_planeCount = 3;
  m_colorMatrix = QMatrix4x4(1.164f, 0.000f, 1.596f, -0.8708f, 1.164f, -0.392f, -0.813f, 0.5296f,
                             1.164f, 2.017f, 0.000f, -1.081f, 0.0f, 0.000f, 0.000f, 1.0000f);

  setFlag(Blending, false);
}

QSGVideoMaterial::~QSGVideoMaterial() {
  if (!m_textureSize.isEmpty()) {
    if (QOpenGLContext *current = QOpenGLContext::currentContext())
      current->functions()->glDeleteTextures(m_planeCount, m_textureIds);
    else
      qWarning() << "QSGVideoMaterial_YUV: Cannot obtain GL context, unable to delete textures";
  }
}

void QSGVideoMaterial::bind() {
  QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
  QSurfaceFormat::OpenGLContextProfile profile =
      QOpenGLContext::currentContext()->format().profile();

  QMutexLocker lock(&m_frameMutex);
  if (m_frame) {
    int fw = m_frame->width;
    int fh = m_frame->height;

    // Frame has changed size, recreate textures...
    if (m_textureSize != m_frame->size()) {
      if (!m_textureSize.isEmpty())
        functions->glDeleteTextures(m_planeCount, m_textureIds);
      functions->glGenTextures(m_planeCount, m_textureIds);
      m_textureSize = m_frame->size();
    }

    GLint previousAlignment;
    const GLenum texFormat1 = (profile == QSurfaceFormat::CoreProfile) ? GL_RED : GL_LUMINANCE;

    functions->glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    functions->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const int y = 0;
    const int u = 1; // m_frame->pixelFormat() == QVideoFrame::Format_YV12 ? 2 : 1;
    const int v = 2; // m_frame->pixelFormat() == QVideoFrame::Format_YV12 ? 1 : 2;

    m_planeWidth[0] = qreal(fw) / m_frame->strides[y];
    m_planeWidth[1] = m_planeWidth[2] = qreal(fw) / (2 * m_frame->strides[u]);

    const int uvHeight = fh / 2; // 420p

    functions->glActiveTexture(GL_TEXTURE1);
    bindTexture(m_textureIds[1], m_frame->strides[u], uvHeight, m_frame->planes[u].get(),
                texFormat1);
    functions->glActiveTexture(GL_TEXTURE2);
    bindTexture(m_textureIds[2], m_frame->strides[v], uvHeight, m_frame->planes[v].get(),
                texFormat1);
    functions->glActiveTexture(GL_TEXTURE0);  // Finish with 0 as default texture unit
    bindTexture(m_textureIds[0], m_frame->strides[y], fh, m_frame->planes[y].get(), texFormat1);

    functions->glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

    m_frame.reset();
  } else {
    // Go backwards to finish with GL_TEXTURE0
    for (int i = m_planeCount - 1; i >= 0; --i) {
      functions->glActiveTexture(GL_TEXTURE0 + i);
      functions->glBindTexture(GL_TEXTURE_2D, m_textureIds[i]);
    }
  }
}

void QSGVideoMaterial::bindTexture(int id, int w, int h, const uchar *bits, GLenum format) {
  QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
  functions->glBindTexture(GL_TEXTURE_2D, id);
  functions->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, bits);
  functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

QSGVideoNode::QSGVideoNode() {
  setFlag(QSGNode::OwnsGeometry);
  setFlag(QSGNode::OwnsMaterial);
  m_material = new QSGVideoMaterial();
  setMaterial(m_material);
}

QSGVideoNode::~QSGVideoNode() {}

/* Helpers */
static inline void qSetGeom(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
  v->x = p.x();
  v->y = p.y();
}

static inline void qSetTex(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
  v->tx = p.x();
  v->ty = p.y();
}

/* Update the vertices and texture coordinates. */
void QSGVideoNode::setTexturedRectGeometry(const QRectF &rect, const QRectF &textureRect)
{
  if (rect == m_rect && textureRect == m_textureRect)
    return;

  m_rect = rect;
  m_textureRect = textureRect;

  QSGGeometry *g = geometry();

  if (g == 0)
    g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

  QSGGeometry::TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();

  // Set geometry first
  qSetGeom(v + 0, rect.topLeft());
  qSetGeom(v + 1, rect.bottomLeft());
  qSetGeom(v + 2, rect.topRight());
  qSetGeom(v + 3, rect.bottomRight());

  // and then texture coordinates
  // tl, bl, tr, br
  qSetTex(v + 0, textureRect.topLeft());
  qSetTex(v + 1, textureRect.bottomLeft());
  qSetTex(v + 2, textureRect.topRight());
  qSetTex(v + 3, textureRect.bottomRight());

  if (!geometry())
    setGeometry(g);

  markDirty(DirtyGeometry);
}

void QSGVideoNode::setCurrentFrame(const SharedVideoFrame &frame) {
  m_material->setCurrentFrame(frame);
  markDirty(DirtyMaterial);
}

void QSGVideoMaterialShader::updateState(const RenderState &state,
                                                      QSGMaterial *newMaterial,
                                                      QSGMaterial *oldMaterial) {
  Q_UNUSED(oldMaterial);

  QSGVideoMaterial *mat = static_cast<QSGVideoMaterial *>(newMaterial);
  program()->setUniformValue(m_id_plane1Texture, 0);
  program()->setUniformValue(m_id_plane2Texture, 1);

  mat->bind();

  program()->setUniformValue(m_id_colorMatrix, mat->m_colorMatrix);
  program()->setUniformValue(m_id_plane1Width, mat->m_planeWidth[0]);
  program()->setUniformValue(m_id_plane2Width, mat->m_planeWidth[1]);
  if (state.isOpacityDirty()) {
    mat->m_opacity = state.opacity();
    program()->setUniformValue(m_id_opacity, GLfloat(mat->m_opacity));
  }
  if (state.isMatrixDirty())
    program()->setUniformValue(m_id_matrix, state.combinedMatrix());

  program()->setUniformValue(m_id_plane3Texture, 2);
  program()->setUniformValue(m_id_plane3Width, mat->m_planeWidth[2]);
}
