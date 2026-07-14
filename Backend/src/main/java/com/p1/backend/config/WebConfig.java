package com.p1.backend.config;

import com.p1.backend.interceptor.JwtAuthInterceptor;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

@Configuration
public class WebConfig implements WebMvcConfigurer {

    private final JwtAuthInterceptor jwtAuthInterceptor;

    public WebConfig(JwtAuthInterceptor jwtAuthInterceptor) {
        this.jwtAuthInterceptor = jwtAuthInterceptor;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        // 로그인/회원가입은 토큰이 없는 상태에서 호출되므로 제외 — 매칭 API만 보호.
        registry.addInterceptor(jwtAuthInterceptor)
                .addPathPatterns("/api/match/**");
    }
}
