package com.p1.backend.interceptor;

import com.p1.backend.service.JwtService;
import io.jsonwebtoken.JwtException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

// Spring Security 없이 /api/match/** 라우트만 보호하는 최소 인터셉터.
// Authorization: Bearer <token> 검증 후 email(로그인 식별자, JWT subject)을 request attribute에
// 심어두면, 컨트롤러가 이를 읽어 "누가 요청했는지" 판별한다.
@Component
public class JwtAuthInterceptor implements HandlerInterceptor {

    public static final String AUTHENTICATED_EMAIL_ATTRIBUTE = "authenticatedEmail";

    private static final String BEARER_PREFIX = "Bearer ";

    private final JwtService jwtService;

    public JwtAuthInterceptor(JwtService jwtService) {
        this.jwtService = jwtService;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) throws Exception {
        String header = request.getHeader("Authorization");
        if (header == null || !header.startsWith(BEARER_PREFIX)) {
            response.sendError(HttpServletResponse.SC_UNAUTHORIZED, "MISSING_TOKEN");
            return false;
        }

        String token = header.substring(BEARER_PREFIX.length());
        try {
            String email = jwtService.parseEmail(token);
            request.setAttribute(AUTHENTICATED_EMAIL_ATTRIBUTE, email);
            return true;
        } catch (JwtException e) {
            response.sendError(HttpServletResponse.SC_UNAUTHORIZED, "INVALID_TOKEN");
            return false;
        }
    }
}
